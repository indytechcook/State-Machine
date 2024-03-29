<?php

class StateMachine {
  protected $states = array();
  protected $events = array();
  protected $initial;
  protected $current;
  protected $object;

  public function __construct($object = NULL) {
    $this->object = $object;
    $this->init();
    $this->set_current_state((string) $this->load());
  }

  /**
   * Initialize the state machine using the provided addState and addEvent
   * methods.
   */
  protected function init() {}

  /**
   * Persist the current state to the object storage.
   */
  protected function persist() {}

  /**
   * Load the current state from the given state storage
   *
   */
  protected function load() {}

  /**
   * Create a new state. This method does not actually create a state instance,
   * it only stores the options array until an instance is requested from
   * get_state().
   *
   * @param $key
   *   The string identifier for this state.  Must be unique within the scope
   *   of the State Machine.
   * @param $options
   *   An array of options that will be passed into the State constructor.
   *   - title: The human-readable title of the state.
   *   - on_enter: An array of callbacks to be fired when entering this state.
   *   - on_exit: An array of callbacks to be fired when exiting this state.
   */
  protected function create_state($key, $options = array()) {
    $this->states[$key] = $options;
  }

  /**
   * Return a state instance by key, lazy-loading the instance if necessary.
   *
   * @param $key
   *   The string identifier of the state to be returned.
   * @return
   *   An instance of StateMachine_State, or FALSE if the key specified does
   *   not represent a valid state.
   */
  protected function get_state($key) {
    if (!array_key_exists($key, $this->states)) {
      return FALSE;
    }

    if (is_array($this->states[$key])) {
      $options = $this->states[$key];
      $this->states[$key] = new StateMachine_State($this, $options);
    }

    return $this->states[$key];
  }

  /**
   * Set the current state to the state identified by the specified key.
   */
  protected function set_current_state($key) {
    if (array_key_exists($key, $this->states)) {
      $this->current = $key;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Returns the current state.
   */
  public function get_current_state() {
    if (!$this->current) {
      $this->current = key($this->states);
    }
    return $this->current;
  }

  /**
   * Set the initial state for this machine.  By default, the initial state is
   * set the the first created state.
   */
  protected function set_initial_state($key) {
    if (array_key_exists($key, $this->states)) {
      $this->initial_state = $key;
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Create a new event. This method does not actually create an event instance,
   * it only stores the options array until an instance is requested from
   * get_event().
   *
   * @param $key
   *   The string identifier for this event.  Must be unique within the scope
   *   of the State Machine.
   * @param $options
   *   An array of options that will be passed into the Event constructor.
   *   - title: The human-readable title of the event.
   *   - origin: A key or array of keys representing the valid origin state(s)
   *     for this event.
   *   - target: The key of the state which this event will transition to.
   *   - guard: A callback to be fired before the event. If this function
   *     returns FALSE, the event will be cancelled.
   *   - before_transition: A callback to be fired after the guard condition,
   *     but before the transition.
   *   - after_transition: A callback to be fired after the completion of the
   *     transition.
   */
  protected function create_event($key, $options = array()) {
    $this->events[$key] = $options;
  }

  /**
   * Return an event instance by key, lazy-loading the instance if necessary.
   *
   * @param $key
   *   The string identifier of the event to be returned.
   * @return
   *   An instance of StateMachine_Event, or FALSE if the key specified does
   *   not represent a valid event.
   */
  protected function get_event($key) {
    if (!array_key_exists($key, $this->events)) {
      return FALSE;
    }

    if (is_array($this->events[$key])) {
      $options = $this->events[$key];
      $this->events[$key] = new StateMachine_Event($this, $options);
    }

    return $this->events[$key];
  }

  /**
   * Trigger an event to process a transition. Callbacks and guard conditions
   * will be processed in the following order:
   * - event:before_transition
   * - event:guard, exits if guard condition return FALSE
   * - oldstate:on_exit
   * - update the current state
   * - newstate:on_enter
   * - event:after_transition
   */
  public function fire_event($key) {
    $event = $this->get_event($key);

    if ($event && ($new_state = $event->execute())) {
      // Allow the previous state to run its 'on_exit' callbacks.
      $this->get_state($this->get_current_state())->on_exit();

      // Set and save the new state.
      $this->set_current_state($new_state);
      $this->persist();

      // Allow the new state to run its 'on_enter' callbacks.
      $this->get_state($this->get_current_state())->on_enter();

      // Allow the event to "finish"
      $event->finish();
    }
    else {
      $this->on_event_fail($event);
      return FALSE;
    }
  }

  /**
   * Method to be called when firing an event fails for any reason.
   */
  protected function on_event_fail($event) {
  }

  /**
   * Returns an array of events that are valid for the current state.
   */
  public function get_available_events() {
    $events = array();
    foreach (array_keys($this->events) as $key) {
      if ($this->get_event($key)->can_transition_from($this->current)) {
        $events[] = $key;
      }
    }
    return $events;
  }
}

class StateMachine_State {
  protected $machine;
  protected $options;

  public function __construct($machine, $options = array()) {
    $this->machine = $machine;
    $this->options = $options;
  }

  /**
   * Called when entering into this state.
   */
  public function on_enter() {
    $args = func_get_args();
    if (!empty($this->options['on_enter'])) {
      call_user_func_array($this->options['on_enter'], $args);
    }
  }

  /**
   * Called when exiting out of this state.
   */
  public function on_exit() {
    $args = func_get_args();
    if (!empty($this->options['on_exit'])) {
      call_user_func_array($this->options['on_exit'], $args);
    }
  }
}

class StateMachine_Event {
  protected $machine;
  protected $options;

  public function __construct($machine, $options = array()) {
    $this->machine = $machine;

    // Normalize the origin option.
    $origin = empty($options['origin']) ? array() : $options['origin'];
    $options['origin'] = is_array($origin) ? $origin : array($origin);

    $this->options = $options;
  }

  /**
   * Validate the transition by processing the guard condition.
   */
  protected function validate() {
    // Execute guard condition if it exists.
    if (!empty($this->options['guard'])) {
      if (call_user_func($this->options['guard'], $this) === FALSE) {
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Execute the event. Tests to make sure the current state is a valid origin
   * state, checks the guard condition,
   */
  public function execute() {
    if (!in_array($this->machine->get_current_state(), $this->options['origin'])) {
      return FALSE;
    }

    if (!$this->validate()) {
      return FALSE;
    }

    if (!empty($this->options['before_transition'])) {
      call_user_func($this->options['before_transition']);
    }

    return $this->options['target'];
  }

  /**
   * Allow the event to finish after the machine has changed state.
   */
  public function finish() {
    if (!empty($this->options['after_transition'])) {
      call_user_func($this->options['after_transition']);
    }
  }

  /**
   * Can this event be used to transition from the specified state?
   */
  public function can_transition_from($state) {
    return in_array($state, $this->options['origin']);
  }
}

