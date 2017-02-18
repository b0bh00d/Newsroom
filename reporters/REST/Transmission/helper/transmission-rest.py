#!/usr/bin/env python
# pylint: disable=missing-docstring
# pylint: disable=bad-whitespace

import os
import re
import sys
import cgi
import json
import argparse
import threading
import subprocess

from SocketServer import ThreadingMixIn
from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer

from daemon import Daemon

ratio_limit = 0.0       # this value says "we don't know"

#-------------------------------------------------------------------------#
#     App: Newsroom                                                       #
#  Module: transmission-rest.py                                           #
#  Author: Bob Hood                                                       #
# License: LGPL-3.0                                                       #
#   PyVer: 2.7.x                                                          #
#  Detail: This module provides a simple web server that feeds the output #
#          of 'transmission-remote --list' to callers as JSON.            #
#-------------------------------------------------------------------------#

# based on:
# https://mafayyaz.wordpress.com/2013/02/08/writing-simple-http-server-in-python-with-rest-and-json/

class HTTPRequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.send_response(400, "Bad POST: Server does not support POST'ing data")
        self.send_header('Content-Type', 'application/json')
        self.end_headers()

    def do_GET(self):
        if self.path.endswith('/api/v1/transmission-rest'):
            # it might be more appropriate in the "REST" sense to
            # actually let the caller append the slot in which they
            # are interested (e.g., "/api/v1/transmission-rest?slot=3"),
            # but that would entail many calls per update just to
            # cover them all, not to mention the fact that we'd have
            # to provide a different function just so they could
            # determine how many active slots there are.  instead,
            # dumping the entire listing not only allows the poller
            # to sort things out, it also provides the knowledge of
            # the number of active slots in one go.
            #
            # this is not an actual REST interface, just a helper.

            self._send_update()
        else:
            self.send_response(403)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()

    def _send_update(self):
        global ratio_limit

        output = subprocess.Popen(['transmission-remote', '--list'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT).communicate()[0]
        if 'command not found' in output:
            self.send_response(400)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write('''{ "type" : "error",
                                  "data" : "The transmission-remote command could not be found",
                                }''')
        else:
            # convert the Transmission status into JSON
            json = '{\n"type" : "status",\n"slots" : [\n'
            count = 0
            for line in output.split('\n'):
                object = ''
                if count:
                    object = ',\n'

                if not line.startswith(' '):
                    continue

                slot   = line[:4].strip()
                done   = line[7:11].strip()
                have   = line[12:22].strip()
                eta    = line[24:32].strip()
                up     = line[34:40].strip()
                down   = line[42:48].strip()
                ratio  = line[50:55].strip()
                status = line[57:69].strip()
                name   = line[70:].strip()

                object = '%s' \
                         '    {\n' \
                         '        "slot" : %s,\n' \
                         '        "done" : "%s",\n' \
                         '        "have" : "%s",\n' \
                         '        "eta" : "%s",\n' \
                         '        "up" : "%s",\n' \
                         '        "down" : "%s",\n' \
                         '        "ratio" : "%s",\n' \
                         '        "status" : "%s",\n' \
                         '        "name" : "%s"\n' \
                         '    }' % \
                       (object, slot, done, have, eta, up,
                        down, ratio, status, name)

                json = '%s%s' % (json, object)

                count += 1

            json = '%s\n    ],\n    "count" : %d,\n    "maxratio" : %g\n}\n' \
                    % (json, count, ratio_limit)

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json)

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    allow_reuse_address = True

    def shutdown(self):
        self.socket.close()
        HTTPServer.shutdown(self)
 
class SimpleHttpServer():
    def __init__(self, ip, port):
        self.server = ThreadedHTTPServer((ip,port), HTTPRequestHandler)
 
    def start(self):
        self.server_thread = threading.Thread(target=self.server.serve_forever)
        self.server_thread.daemon = True
        self.server_thread.start()
 
    def waitForThread(self):
        self.server_thread.join()
 
    def stop(self):
        self.server.shutdown()
        self.waitForThread()
 
class MyDaemon(Daemon):
    def __init__(self, ip='', port=8000):
        super(MyDaemon, self).__init__('/tmp/%s.pid' % os.path.basename(__file__).split('.')[0])

        self.ip = ip
        self.port = port

    def run(self):
        server = SimpleHttpServer(self.ip, self.port)
        server.start()
        server.waitForThread()

if __name__ == '__main__':
    #parser = argparse.ArgumentParser(description='HTTP Server')
    #parser.add_argument('--port', default=8000, type=int, help='Listening port for HTTP Server')
    #parser.add_argument('--ip', default='', help='HTTP Server IP')
    #parser.add_argument('start', action='store_true', default=False, help='HTTP Server IP')
    #parser.add_argument('stop', action='store_true', default=False, help='HTTP Server IP')
    #parser.add_argument('restart', action='store_true', default=False, help='HTTP Server IP')
    #args = parser.parse_args()
    #print args
    #sys.exit(0)

    if os.path.exists('/etc/transmission-daemon/settings.json'):
        try:
            # typically, this file will be owned by root and not have
            # read permissions for any other user (due to potential
            # password embedding).  if you are on a closed system (i.e.,
            # no possible way your Transmission machine can be accessed
            # from outside your network), you might want to enable read
            # access to the file instead of running this script as root.

            data = open('/etc/transmission-daemon/settings.json').read()
        except IOError:
            pass
        else:
            settings = json.loads(data)
            ratio_limit = float(settings["ratio-limit"])

    daemon = MyDaemon()

    if len(sys.argv) == 2:
        if 'start' == sys.argv[1]:
            daemon.start()
        elif 'stop' == sys.argv[1]:
            daemon.stop()
        elif 'restart' == sys.argv[1]:
            daemon.restart()
        else:
            print "Unknown command"
            sys.exit(2)
        sys.exit(0)
    else:
        print "usage: %s start|stop|restart" % sys.argv[0]
        sys.exit(2)
