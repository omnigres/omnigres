# Technical Decisions

The documents in this folder serve as a reference for important technical decisions made in the design and
implementation of schema diff feature of omni_schema.

Code and tests are highly encouraged to refer to particular technical decisions.

Technical decisions are identified by their unique codes and are never retired from the list for the reasons of tracking
past decisions. If a new decision overrides an old one, both have to be cross-linked. They can be renamed but the
identifier code should not change.

Technical decisions can also be withdrawn (but not removed for historical reasons) if they are found to be incorrect,
ineffective, or counter-effective.

Any explanation for how the schema diff behaves should be traceable to these decisions.

## SDR: Schema Diff Rules

Rules that govern schema diff feature of omni_schema.

| TD                         | Description                     |
|----------------------------|---------------------------------|
| [SDR_0001.md](SDR_0001.md) | Create/Delete table             |
| [SDR_0002.md](SDR_0002.md) | Create/Delete column in a table |
| [SDR_0003.md](SDR_0003.md) | Change in column attribute      |