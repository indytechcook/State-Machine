Finite State Machine
--------------------
A machine is a device which accepts some inputs, possibly produces some output,
and has some memory sorting information on the overall results of all previous
inputs.

The condition of the machine at a particular instance is called a state of the
machine. New input changes the state, may be to another state, and may be to
the same state. The effect of a new input depends on the present state the
machine is in.

If the total number of possible states is finite, the machine is called finite
state machine, eg. a computer.



Event Execution Summary
-----------------------
1. Check the event's guard condition.  Stop here if FALSE.
2. Event::before_transition
3. CurrentState::on_exit
4. Update state in machine
5. NewState::on_enter
6. Event::after_transition
