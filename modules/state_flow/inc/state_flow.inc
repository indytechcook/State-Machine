<?php
// $Id$

/**
 * @file
 * State Flow implementation of the State Machine class
 */

state_machine_load_class_file();

class StateFlow extends StateMachine {
  /**
   * Called from StateMachine::__construct to initialize the states and events.
   * Define two states.
   * First revision:
   *  - Expose go to draft button
   *  - Expose go to publish button
   *  - Upon publish, create new revision (handled outside of state machine)
   * Second revision:
   *  - Menu alter edit link to load most recent revision (whether published or revision from states)
   *  - On hook_nodeapi (op: load), force new revision checkbox on node edit form
   *    - Expose go to draft button
   *  - Create new revision, prevent node table from updating new revision as published revision
   *  - Expose go to publish button
   *  - Upon publish, set revision id in node table
   *  - Repeat as needed
   */
  public function init() {
    // Initialize states
    $this->create_state('draft');
    $this->create_state('published', array(
      'on_enter' => array($this, 'on_enter_published'),
      'on_exit' => array($this, 'on_exit_published'),
    ));

    // Initialize events
    $this->create_event('publish', array(
      'origin' => 'draft',
      'target' => 'published',
    ));
  }

  public function on_enter_published() {
    $this->set_published();
    $this->set_node_revision();
    $this->set_principle_revision();
  }

  public function on_exit_published() {
    $this->set_unpublished();
  }

  public function persist() {
    $vid = $this->get_latest_revision($this->object->nid);
    $nid = $this->object->nid;
    $state = $this->get_current_state();

    $data = array(
      'vid' => $vid,
      'nid' => $nid,
      'state' => $state,
      'timestamp' => time(),
      'status' => 1,
    );
    $update = ($this->existing_revision($nid, $vid)) ? array('vid') : NULL;
    $result = drupal_write_record('node_revision_states', $data, $update);
    return;
  }

  public function load() {
    return $this->latest_state($this->object->nid);
  }

  public function set_published() {
    $this->object->status = 1;
    node_save($this->object);
  }

  public function set_unpublished() {
    $this->object->status = 0;
    node_save($this->object);
  }

  public function set_node_revision() {
    $vid = $this->get_latest_revision($this->object->nid);
    if (!empty($vid) && ($vid != $this->object->vid)) {
      $this->object->vid = $vid;
      node_save($this->object);
    }
  }

  public function set_principle_revision() {
    $nid = $this->object->nid;
    $vid = $this->get_latest_revision($nid);
    $result = db_result(db_query('UPDATE {node_revision_states} SET status = 0 WHERE nid = %d AND vid != %d', $nid, $vid));
    return;
  }

  public function get_latest_revision($nid) {
    $result = db_result(db_query_slave('SELECT MAX(vid) FROM {node_revisions} WHERE nid = %d', $nid));
    return $result;
  }

  public function get_current_revision($nid) {
    $result = db_result(db_query_slave('SELECT vid FROM {node_revision_states} WHERE nid = %d ORDER BY timestamp DESC', $nid, 0, 1));
    return $result;
  }

  public function existing_revision($nid, $vid) {
    $result = db_result(db_query_slave('SELECT COUNT(*) FROM {node_revision_states} WHERE nid = %d AND vid = %d', $nid, $vid));
    return ($result) ? TRUE : FALSE;
  }

  public function latest_state($nid) {
    $query = "SELECT state
              FROM {node_revision_states}
              WHERE nid = %d AND status = 1 AND vid = %d
              ORDER BY timestamp
              DESC
              LIMIT 1";
    return db_result(db_query($query, $nid, $this->get_latest_revision($nid)));
  }
}