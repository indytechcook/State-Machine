<?php
// $Id$

/**
 * @file
 * State Flow implementation of the State Machine class
 */

state_machine_load_class_file();

class StateFlow extends StateMachine {
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
    drupal_write_record('node', $this->object, 'nid');
  }

  public function set_unpublished() {
    $this->object->status = 0;
    drupal_write_record('node', $this->object, 'nid');
  }

  public function set_node_revision() {
    $vid = $this->get_latest_revision($this->object->nid);
    if (!empty($vid) && ($vid != $this->object->vid)) {
      $this->object->vid = $vid;
      drupal_write_record('node', $this->object, 'nid');
    }
  }

  public function set_principle_revision() {
    $nid = $this->object->nid;
    $vid = $this->get_latest_revision($nid);
    $result = db_query('UPDATE {node_revision_states} SET status = 0 WHERE nid = %d AND vid != %d', $nid, $vid);
    return;
  }

  public function get_latest_revision($nid) {
    $result = db_result(db_query_slave('SELECT MAX(vid) FROM {node_revisions} WHERE nid = %d', $nid));
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
    return db_result(db_query_slave($query, $nid, $this->get_latest_revision($nid)));
  }
}

class StateFlowEvent extends StateMachine_Event {
  public function validate() {
    if(parent::validate()) {
      if (!empty($this->options['permission'])) {
        if (call_user_func($this->options['permission'], $this) === FALSE) {
          return FALSE;
        }
      }
      return TRUE;
    }
    return FALSE;
  }
}