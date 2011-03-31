<?php

require_once('../../../inc/base.inc');

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
      'on_enter' => array($this, 'set_published', 'set_node_revision', 'set_principle_revision'),
      'on_exit' => array($this, 'set_unpublished'),
    ));

    // Draft to Publish
    $this->create_event('to_publish', array(
      'origin' => 'draft',
      'target' => 'published',
    ));

    // Publish to Draft
    $this->create_event('to_draft', array(
      'origin' => 'published',
      'target' => 'draft',
    ));
  }

  public function set_published() {

  }

  public function set_node_revision() {

  }

  public function set_principle_revision() {

  }
}
