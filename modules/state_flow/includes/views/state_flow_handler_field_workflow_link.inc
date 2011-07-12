<?php

/**
 * Views field handler to display a link to the workflow page for the node.
 */
class state_flow_handler_field_workflow_link extends views_handler_field {
  function query() {
    // Override this method.
  }

  function render($values) {
    return l(t('Workflow'), 'node/' . $values->nid . '/workflow');
  }
}
