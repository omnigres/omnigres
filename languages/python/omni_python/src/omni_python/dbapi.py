"""
plpydbapi

Copyright (c) 2011, Peter Eisentraut

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written
agreement is hereby granted, provided that the above copyright notice
and this paragraph and the following two paragraphs appear in all
copies.

IN NO EVENT SHALL THE AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT,
INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
DOCUMENTATION, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
BASIS, AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

(The PostgreSQL License, http://www.opensource.org/licenses/postgresql)
"""

"""
A Python DB-API compatible (sort of) interface on top of PL/Python
"""

__author__ = "Peter Eisentraut <peter@eisentraut.org>"


import decimal
import plpy
import sys
import time


__plpy = sys.modules['__main__'].plpy


## Module Interface


def connect():
    return Connection()


apilevel = '2.0'
threadsafety = 0  # Threads may not share the module.
paramstyle = 'format'


if sys.version[0] == '3':
    long = int
    StandardError = Exception


class Warning(StandardError):
    pass


class Error(StandardError):
    def __init__(self, spierror=None):
        super(Error, self).__init__()
        self.spierror = spierror


class InterfaceError(Error):
    pass


class DatabaseError(Error):
    pass


class DataError(DatabaseError):
    pass


class OperationalError(DatabaseError):
    pass


class IntegrityError(DatabaseError):
    pass


class InternalError(DatabaseError):
    pass


class ProgrammingError(DatabaseError):
    pass


class NotSupportedError(DatabaseError):
    pass

## Connection Objects


class Connection:
    Warning = Warning
    Error = Error
    InterfaceError = InterfaceError
    DatabaseError = DatabaseError
    DataError = DataError
    OperationalError = OperationalError
    IntegrityError = IntegrityError
    InternalError = InternalError
    ProgrammingError = ProgrammingError
    NotSupportedError = NotSupportedError

    closed = False
    autocommit = True


    _subxact = None

    def __init__(self):
        pass

    def close(self):
        if self.closed:
            raise Error()
        self.rollback()
        self.closed = True

    def _ensure_transaction(self):
        if self._subxact is None:
            self._subxact = plpy.subtransaction()
            self._subxact.enter()

    def commit(self):
        if self.closed:
            raise Error()
        if self._subxact is not None:
            self._subxact.exit(None, None, None)
        self._subxact = None

    def rollback(self):
        if self.closed:
            raise Error()
        if self._subxact is not None:
            self._subxact.exit('fake exception', None, None)
        self._subxact = None

    def cursor(self):
        newcursor = Cursor()
        newcursor.connection = self
        return newcursor


## Cursor Objects


class Cursor:
    description = None
    rowcount = -1
    arraysize = 1
    rownumber = None
    closed = False
    connection = None

    _execute_result = None

    _SPI_OK_UTILITY = 4
    _SPI_OK_SELECT = 5
    _SPI_OK_INSERT_RETURNING = 11
    _SPI_OK_DELETE_RETURNING = 12
    _SPI_OK_UPDATE_RETURNING = 13

    def __init__(self):
        pass

    def close(self):
        self.closed = True

    def _is_closed(self):
        return self.closed or self.connection.closed

    def execute(self, operation, parameters=None):
        if self._is_closed():
            raise Error()

        self.connection._ensure_transaction()

        parameters = parameters or []

        placeholders = []
        types = []
        values = []
        for i, param in enumerate(parameters):
            placeholders.append("$%d" % (i + 1))
            types.append(self.py_param_to_pg_type(param))
            values.append(param)
        try:
            query = operation % placeholders
            plan = plpy.prepare(query, types)
            res = plpy.execute(plan, values)
        except plpy.SPIError as e:
            raise Error(e)

        self._execute_result = None
        self.rownumber = None
        self.description = None
        self.rowcount = -1


        if res.status() in (self._SPI_OK_SELECT, self._SPI_OK_INSERT_RETURNING, self._SPI_OK_DELETE_RETURNING, self._SPI_OK_UPDATE_RETURNING):
            self._execute_result = [[row[col] for col in row] for row in res]
            self.rownumber = 0
            if 'colnames' in res.__class__.__dict__:
                # PG 9.2+: use .colnames() and .coltypes() methods
                self.description = [(name, get_type_obj(typeoid), None, None, None, None, None) for name, typeoid in zip(res.colnames(), res.coltypes())]
            elif len(res) > 0:
                # else get at least the column names from the row keys
                self.description = [(name, None, None, None, None, None, None) for name in res[0].keys()]
            else:
                # else we know nothing
                self.description = [(None, None, None, None, None, None, None)]

        if res.status() == self._SPI_OK_UTILITY:
            self.rowcount = -1
        else:
            self.rowcount = res.nrows()

        if self.connection.autocommit == True:
            self.connection.commit()

    @staticmethod
    def py_param_to_pg_type(param):
        if isinstance(param, bool):
            pgtype = 'bool'
        elif isinstance(param, decimal.Decimal):
            pgtype = 'numeric'
        elif isinstance(param, float):
            pgtype = 'float8'
        elif isinstance(param, long):
            pgtype = 'int'
        elif isinstance(param, int):
            pgtype = 'int'
        else:
            pgtype = 'text'
        # TODO ...
        return pgtype

    def executemany(self, operation, seq_of_parameters):
        # We can't reuse saved plans here, because we have no way of
        # knowing whether all parameter sets will be of the same type.
        totalcount = 0
        for parameters in seq_of_parameters:
            self.execute(operation, parameters)
            if totalcount != -1:
                totalcount += self.rowcount
        self.rowcount = totalcount

    def fetchone(self):
        if self._execute_result is None:
            raise Error()
        if self.rownumber == len(self._execute_result):
            return None
        result = self._execute_result[self.rownumber]
        self.rownumber += 1
        return result

    def fetchmany(self, size=None):
        if self._execute_result is None:
            raise Error()
        if size is None:
            size = self.arraysize
        result = self._execute_result[self.rownumber:self.rownumber + size]
        self.rownumber += size
        return result

    def fetchall(self):
        if self._execute_result is None:
            raise Error()
        result = self._execute_result[self.rownumber:]
        self.rownumber = len(self._execute_result)
        return result

    def next(self):
        result = self.fetchone()
        if result is None:
            raise StopIteration
        return result

    def scroll(self, value, mode='relative'):
        if mode == 'relative':
            newpos = self.rownumber + value
        elif mode == 'absolute':
            newpos = value
        else:
            raise ValueError("Invalid mode")
        if newpos < 0 or newpos > len(self._execute_result):
            raise IndexError("scroll operation would leave result set")
        self.rownumber = newpos

    def setinputsizes(self, sizes):
        pass

    def setoutputsize(self, size, column=None):
        pass

    def __iter__(self):
        return self


## Type Objects and Constructors


def Date(year, month, day):
    return '%04d-%02d-%02d' % (year, month, day)


def Time(hour, minute, second):
    return '%02d:%02d:%02d' % (hour, minute, second)


def Timestamp(year, month, day, hour, minute, second):
    return '%04d-%02d-%02d %02d:%02d:%02d' % (year, month, day, hour, minute, second)


def DateFromTicks(ticks):
    return Date(*time.localtime(ticks)[:3])


def TimeFromTicks(ticks):
    return Time(*time.localtime(ticks)[3:6])


def TimestampFromTicks(ticks):
    return Timestamp(*time.localtime(ticks)[:6])


def Binary(string):
    return string


# Type objects


class STRING:
    pass


class BINARY:
    pass


class NUMBER:
    pass


class DATETIME:
    pass


class ROWID:
    pass


_typname_typeobjs = {
    'bytea': BINARY,
}


_typcategory_typeobjs = {
    'D': DATETIME,
    'N': NUMBER,
    'S': STRING,
    'T': DATETIME,
}


_typoid_typeobjs = {}


def get_type_obj(typeoid):
    """Return the type object (STRING, NUMBER, etc.) that corresponds
    to the given type OID."""
    if not _typoid_typeobjs:
        for row in plpy.execute(plpy.prepare("SELECT oid, typname, typcategory FROM pg_type")):
            if row['typcategory'] in _typcategory_typeobjs:
                _typoid_typeobjs[int(row['oid'])] = _typcategory_typeobjs[row['typcategory']]
            elif row['typname'] in _typname_typeobjs:
                _typoid_typeobjs[int(row['oid'])] = _typname_typeobjs[row['typname']]

    return _typoid_typeobjs.get(typeoid)
