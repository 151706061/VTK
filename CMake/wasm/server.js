// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause\
//
// Usage: node server.js -d,--directory /path/to/Testing/Temporary -p,--port N -o,--operation {start,run,stop}
//
// # Operations (-o/--operation)
//
// In the "start" operation, we fork a new 'detached' node process that launches
// this script with "-o run" argument. We wait for the child process to
// send a "listening" message informing us that the server is now listening for connections,
// and a "vtkhttp.lock" file has been created. We exit upon receiving that information.
//
// In the "run" operation, we create a HTTP server and
// write details of the HTTP Server and our PID into a "vtkhttp.lock" file.
// We send a message "listening" via IPC to the parent process so that it can
// safely exit.
//
// The "stop" operation kills the server process by reading the pid from the "vtkhttp.lock" file.
// It subsequently removes the lock file so that future server starts succeed.

import http from 'node:http';
import { URL } from 'node:url';
import fs from 'node:fs';
import { parseArgs } from 'node:util';
import process from 'node:process';
import path from 'node:path';
import { fork } from 'node:child_process';

const OPERATIONS =
{
  START: 'start',
  RUN: 'run',
  STOP: 'stop',
};

const {
  values: { directory, port, operation },
} = parseArgs({
  options: {
    directory: {
      type: 'string',
      short: 'd',
      default: process.cwd()
    },
    port: {
      type: 'string',
      short: 'p',
      default: '8000'
    },
    operation: {
      type: 'string',
      short: 'o',
    },
  },
});

const HOST = '127.0.0.1';
const PORT = Number.parseInt(port, 10);
const OPERATION = operation;
const LOCK = path.join(directory, 'vtkhttp.lock');

if (OPERATION == OPERATIONS.START) {
  console.log('starting server process..');
  if (fs.existsSync(LOCK)) {
    console.error(`lock file ${LOCK} exists. Run with -o stop to kill existing HTTP server and try again.`)
    process.exit(1);
  }
  fork(process.argv[1],
    ['-d', directory, '-p', port, '-o', OPERATIONS.RUN],
    {
      detached: true,
      stdio: [
        null,
        fs.openSync(path.join(directory, 'server.out'), 'w'),
        fs.openSync(path.join(directory, 'server.err'), 'w'),
        'ipc',
      ]
    })
    .on('message', (msg) => {
      console.log('received message ', msg);
      if (msg == 'listening') {
        process.exit(0);
      }
    })
    .on('spawn', () => {
      console.log('server process spawned.');
    });
} else if (OPERATION == OPERATIONS.STOP) {
  console.log('stopping server process..');
  const vtkhttp = JSON.parse(fs.readFileSync(LOCK));
  process.kill(vtkhttp.pid);
  console.log(`killed ${vtkhttp.pid}`);
  fs.rmSync(LOCK, { force: true, maxRetries: 10 });
} else if (OPERATION == OPERATIONS.RUN) {
  // Create a local server to receive data from
  const server = http.createServer((incomingMesssage, response) => {
    const url = new URL(incomingMesssage.url, `http://${incomingMesssage.headers.host}/`);
    console.debug(`${incomingMesssage.method} ${url.toString()}`);
    if (
      url.pathname === '/dump' &&
      incomingMesssage.method === 'POST'
    ) {
      const query = new URLSearchParams(url.search);
      if (query.has('file')) {
        incomingMesssage.pipe(fs.createWriteStream(query.get('file'))
          .on('finish', () => {
            const body = Buffer.from('OK');
            response
              .writeHead(200, {
                'Content-Length': Buffer.byteLength(body),
                'Content-Type': 'text/plain'
              })
              .end(body);
          })
          .on('error', (err) => {
            const body = Buffer.from(`Internal server error ${err.name}, ${err.message}`);
            response
              .writeHead(500, {
                'Content-Length': Buffer.byteLength(body),
                'Content-Type': 'text/plain'
              })
              .end(body);
          }));
      } else {
        const body = Buffer.from('Invalid query for /dump, expects /dump?file="/path/to/filename.ext"');
        response
          .writeHead(400, {
            'Content-Length': Buffer.byteLength(body),
            'Content-Type': 'text/plain',
          })
          .end(body);
      }
    } else {
      response.writeHead(403, 'Forbidden').end();
    }
  });

  server.listen(PORT, HOST)
    .on('connection', () => {
      console.debug('someone connected');
    })
    .on('error', (err) => {
      if (err.code === 'EADDRINUSE') {
        console.error('address in use, retrying...');
        setTimeout(() => {
          server.close();
          server.listen(PORT, HOST);
        }, 1000);
      }
    })
    .on('listening', () => {
      console.debug('socket listening');
      fs.writeFileSync(
        LOCK,
        JSON.stringify({
          ...server.address(),
          pid: process.pid,
        }));
      process.send('listening');
    });
} else {
  console.error(`unknown value for -o/--operation '${OPERATION}'`)
}
