#!/usr/bin/env ruby

def parse_timeline(filename)
  name_col = 1
  tid_col = 2
  t0_col = 3
  t1_col = 4

  cols_idx = [
    name_col,
    tid_col,
    t0_col,
    t1_col,
  ]

  events = []
  for line in open(filename).each_line
    if line.start_with? "``"
      parts = line.split(" ")
      name, tid, t0, t1 = cols_idx.collect { |i| parts[i] }
      events << [name, tid, t0.to_i, t1.to_i]
    end
  end
  events
end

def query_timeline(events, p1, p2)
  _, _, t0, t1 = events[0]
  for _, _, x, y in events
    t0 = x if x < t0
    t1 = y if y > t1
  end

  tail = []
  for name, tid, x, y in events
    q1 = ((x - t0).to_f / (t1 - t0).to_f) * 100.0
    q2 = ((y - t0).to_f / (t1 - t0).to_f) * 100.0
    if p1 <= q1 and q2 <= p2
      tail << [name, tid, x, y]
    end
  end
  tail
end

def save_events(filename, events)
  open(filename, "w") do |f|
    for name, tid, x, y in events
      f.write("%d %d %s @ %s\n" % [x, y, name, tid])
    end
  end
end

def query(input, output)
  events = parse_timeline input
  tail = query_timeline events, 99.5, 99.54
  save_events(output, tail)
end

query "logs/profile/1024x100/127.0.0.1.10000.stdout.log", "window.0.events"
query "logs/profile/1024x100/127.0.0.1.10001.stdout.log", "window.1.events"
query "logs/profile/1024x100/127.0.0.1.10002.stdout.log", "window.2.events"
query "logs/profile/1024x100/127.0.0.1.10003.stdout.log", "window.3.events"

puts `env LD_LIBRARY_PATH=$HOME/local/opencv/lib vis-interval 'window.0.events,window.1.events,window.2.events,window.3.events' window.png 27,6,6,6 1 2 10`
