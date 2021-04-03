#!/usr/bin/env ruby
require "json"
o = JSON.load(open("compile_commands.json"))
for c in o
  parts = c["command"].split
  for s in parts
    p s
  end
  puts
  puts
end
