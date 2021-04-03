#!/usr/bin/env ruby

def group_log(filename)
  ts = {}
  for line in open(filename).each_line
    tid = line.split(" ")[1]
    ts[tid] = [] unless ts[tid]
    ts[tid] << line.strip
  end

  for tid, lines in ts
    for line in lines
      puts line
    end
    puts
    puts
  end
end

for f in ARGV
  puts "FILE :: %s" % [f]
  group_log f
  puts "EOF :: %s" % [f]
  puts
end
