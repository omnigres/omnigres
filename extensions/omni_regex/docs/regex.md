# Regex

This extensions provides [PCRE2](https://github.com/PCRE2Project/pcre2)-based regular expression functionality.
PCRE2 is feature-rich (for example, it provides named capture groups) and performant.

# Credits

The extension is a fork of [pgpcre](https://github.com/petere/pgpcre) by
Peter Eisentraut with [unmerged](https://github.com/petere/pgpcre/pull/9) contributions from Christoph Berg (pcre2
support), modified to support named capture groups, parallelization and PCRE2.

The original code is licensed under the terms of The PostgreSQL License reproduced below.

??? tip "License"

    Copyright © 2013, Peter Eisentraut <peter@eisentraut.org>

    (The PostgreSQL License)
    
    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose, without fee, and without a written
    agreement is hereby granted, provided that the above copyright notice
    and this paragraph and the following two paragraphs appear in all
    copies.
    
    IN NO EVENT SHALL THE AUTHOR(S) OR ANY CONTRIBUTOR(S) BE LIABLE TO ANY
    PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
    DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
    SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR(S) OR
    CONTRIBUTOR(S) HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
    THE AUTHOR(S) AND CONTRIBUTOR(S) SPECIFICALLY DISCLAIM ANY WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
    PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR(S) AND
    CONTRIBUTOR(S) HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
    UPDATES, ENHANCEMENTS, OR MODIFICATIONS.