# Introduction

`omni_worker` provides a generalized Postgres worker pool that can execute arbitrary workloads within individual
backend contexts. Its architecture allows other extensions to add native-compiled handlers for arbitrary messages. The
simplest example of such a handler is a [SQL execution handler](sql_handler.md) that takes SQL statements and schedules
them to be executed. 
