#!/usr/bin/env ruby

for link in Dir.glob("CMakeFiles/*.dir/link.txt")
  p link
  for s in open(link).read.split
    p s
  end
  puts
  puts
end
