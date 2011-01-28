#!/usr/bin/env python

import cgi

form = cgi.FieldStorage()

result = 0
if 'value' in form:
    for value in form['value']:
        result += int(value.value)
print "Content-Type: text/html"
print "Content-Length: %d\r\n" % (len(str(result)) + 1)
print result
