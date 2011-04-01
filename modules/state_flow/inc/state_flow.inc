<?php

require_once('../../../inc/base.inc');

class StateFlow extends StateMachine {

  //Variables to use throughout functions
  protected $vid;
  protected $current_state;

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
      'on_enter' => array($this, 'set_published', 'set_node_revision', 'set_principle_revision'),
      'on_exit' => array($this, 'set_unpublished'),
    ));

    // Initialize events
    $this->create_event('to_publish', array(
      'origin' => 'draft',
      'target' => 'published',
    ));

    //Set commonly used values
    $this->vid = $this->get_latest_revision($this->node->nid);
    $this->current_state = $this->get_current_state();
  }

  public function persist() {
    $result = drupal_write_record('node_revision_states', array($this->vid, $this->node->nid, $this->current_state, time(), 1));
    return ($result !== FALSE) ? TRUE: FALSE;
  }

  public function set_published() {
    $this->node->status = 1;
    node_save($this->node);
  }

  public function set_unpublished() {
    $this->node->status = 0;
    node_save($this->node);
  }

  public function set_node_revision() {
    $this->node->vid = $$this->vid;
    node_save($this->node);
  }

  public function set_principle_revision() {
    $result = db_result(db_query('UPDATE {node_revision_states} SET status = 0 WHERE nid = %d AND vid != %d', $this->node->nid, $this->vid));
    return;
  }

  public function get_latest_revision($nid) {
    return db_result(db_query_slave('SELECT MAX(vid) FROM {node_revisions} WHERE nid = %d', $nid));
  }

}
