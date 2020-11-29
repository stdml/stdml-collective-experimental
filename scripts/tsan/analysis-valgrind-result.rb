#!/usr/bin/env ruby

require "rexml/document"

doc = REXML::Document.new(open(ARGV[0]))

e = doc.root.elements["error"]
if e
  puts "leak detected"
  #   puts e.methods
  #   for x in e.children
  #     puts x.class
  #     # for s in x.elements["stack"]
  #     #   puts s
  #     # end
  #   end
  exit 1
end

puts "OK"
