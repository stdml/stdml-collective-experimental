#!/usr/bin/env ruby
require "yaml"

def load_db
  std_headers = {}
  open("std.txt").each_line do |line|
    parts = line.strip.split " "
    if parts.count == 2
      k, v = parts
      std_headers[k] = v
    end
  end
  std_headers
end

def f2h_to_h2f
  f2h = load_db

  h2f = {}
  f2h.each do |k, v|
    h2f[v] = [] unless h2f[v]
    h2f[v] << k
  end

  h2f.each do |k, vs|
    h2f[k] = vs.sort
  end

  open("headers.yaml", "w") { |f| f.write YAML.dump(h2f) }
end

f2h_to_h2f
