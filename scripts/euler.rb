#!/usr/bin/env ruby

def reduce_file(filename)
  completed = {}
  lines = []
  for line in open(filename).each_line
    line.strip!
    lines << line
    parts = line.split " "
    if parts[2] == "will complete"
      # parts[3] is promised
    end
    if parts[2] == "completed"
      completed[parts[3]] = true
    end
  end
  for c, _ in completed
    p c
  end
  for line in lines
    f = false
    for c, _ in completed
      if line.include? c
        f = true
        break
      end
    end
    puts line unless f
  end
end

reduce_file ARGV[0]
