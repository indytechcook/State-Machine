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

    $data = new stdClass();
    $data->vid = $vid;
    $data->nid = $nid;
    $data->state = $state;
    $data->timestamp = time();
    $data->status = 1;

    $update = ($this->existing_revision($nid, $vid)) ? array('vid') : array();
    $result = drupal_write_record('node_revision_states', &$data, $update);
    return;
  }

  public function load() {
    $state = $this->latest_state($this->object->nid);
    return $state;
  }

  public function set_published() {
    $this->object->status = 1;
    drupal_write_record('node', $this->object, array('nid'));
  }

  public function set_unpublished() {
    $this->object->status = 0;
    drupal_write_record('node', $this->object, array('nid'));
  }

  public function set_node_revision() {
    $vid = $this->get_latest_revision($this->object->nid);
    if (!empty($vid) && ($vid != $this->object->vid)) {
      $this->object->vid = $vid;
      drupal_write_record('node', $this->object, array('nid'));
      $result = db_update('node_revision')
        ->fields(array(
          'status' => 1,
        ))
        ->condition('vid', $vid)
        ->execute();
    }
  }

  public function set_principle_revision() {
    $nid = $this->object->nid;
    $vid = $this->get_latest_revision($nid);
    $result = db_update('node_revision_states')
      ->fields(array(
        'status' => 0,
      ))
      ->condition('nid', $nid)
      ->condition('vid', $vid, '!=')
      ->execute();
    return;
  }

  public function get_latest_revision($nid) {
    $result = db_query('SELECT MAX(vid) FROM {node_revision} WHERE nid = :nid', array(':nid' => $nid))->fetchCol('vid');
    return $result[0];
  }

  public function existing_revision($nid, $vid) {
    $result = db_select('node_revision_states', 'nrs')
      ->fields('nrs')
      ->condition('nid', $nid)
      ->condition('status', 1)
      ->condition('vid', $vid)
      ->countQuery()
      ->execute()
      ->fetchAll();
    return ($result[0]->expression) ? TRUE : FALSE;
  }

  public function latest_state($nid) {
    $state = NULL;
    $latest_state = db_select('node_revision_states', 'nrs')
      ->fields('nrs', array('state'))
      ->condition('nid', $nid)
      ->condition('status', 1)
      ->condition('vid', $this->get_latest_revision($nid))
      ->orderBy('timestamp', 'DESC')
      ->range(0, 1)
      ->execute()
      ->fetchCol('state');
    if(is_array($latest_state) && (array_key_exists(0, $latest_state))) {
      $state = $latest_state[0];
    }
    return $state;
  }
}