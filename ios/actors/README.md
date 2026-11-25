# Actor-to-UI boundary

Background Atto actors may enqueue only serialized intents and receive acknowledgements. A trusted host constructs actor identity/grants, checks bounded mailbox admission and orders revisions. It posts to the main actor, then RECHECKS current lifecycle, ownership, authorization and revision. Do not hold borrow/pinned UI references across suspension. Scheduler and actor lowering are pending independent compiler/runtime work.
