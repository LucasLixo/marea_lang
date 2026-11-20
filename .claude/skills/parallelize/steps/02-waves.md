# Step 2 of 6 - Build the waves

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

Topologically layer the graph: **wave 1** is every task with no `needs:`; **wave N** is every
task whose `needs:` are all in earlier waves. Tasks inside a wave run in parallel; a wave starts
only when the previous one has fully reported. Show the plan as a table (id, title, wave, needs,
writes, isolation) plus a one-line reason for every `needs:` edge, then proceed - do not wait for
approval unless a rule below says to stop.

Stop and ask the user only when: the request is ambiguous in a way that changes the split, a task
would need something the environment plainly lacks (a display, sudo, docker, network) with no
route around it, or the plan would rewrite files outside what the request names.

## Done when

- The plan table (id, title, wave, needs, writes, isolation) is shown, with a one-line reason per `needs:` edge.
- You stopped to ask only if one of the stop conditions of this step holds.

Next: [steps/03-isolation.md](03-isolation.md)
