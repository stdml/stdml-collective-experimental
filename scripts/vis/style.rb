#!/usr/bin/env ruby

names = [
    'send',

    'write_body',

    'read_body',
    'run_graphs0',
    'run_graphs1',
    'recv_into',
    'send_into',
    'recv_onto',
    'send_onto',

    'add_to',
    'in_add_to',
    'q->get()',
]

n = 4

for name in names do
    for i in 0 .. n do
        key = '%s_%d' %[name, i]
        puts '%s %s' % [key, 'black']
    end
end
