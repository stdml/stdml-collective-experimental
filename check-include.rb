#!/usr/bin/env ruby
require "yaml"
require "json"

$modules = [
  # "logger",
  # "stdapp",
  # "stdml-collective-experimental",
  # "stdml-datasets",
  # "stdml-eval",
  # "stdml-ir-experimental",
  "stdml-operators",
# "stdnn-ops-cuda",
# "stdnn-ops",
# "stdtensor-tools",
# "stdtensor",
# "stdtracer",
]

def module_sources(m)
  [
    "include",
    "src",
  #  "examples",
  ].collect { |d| m + "/" + d }
end

def extract_includes(filename)
  incs = []
  for line in open(filename).each_line
    if line =~ /^\#include/
      incs << line.strip
    end
  end
  # incs = incs - $cheaders - $cppheaders
  incs.collect do |i|
    if i =~ /\<(.+)\>/
      $1
    elsif i =~ /\"(.+)\"/
      $1
    else
      puts "can't parse include #{i}"
      nil
    end
  end
end

def extract_std_tokens(filename)
  open(filename).each_line.collect do |line|
    p = /(std::[:\w]+|size_t|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|printf|fprintf)/
    line.scan(p).flatten
  end.flatten.uniq.compact
end

def list_sources
  roots = module_sources(".") #  + $modules.collect { |m| module_sources("vendors/" + m) }.flatten
  patterns = roots.collect { |r| ["#{r}/**/*.h", "#{r}/**/*.hpp", "#{r}/**/*.cpp"] }.flatten
  Dir.glob(patterns)
end

def extract_all_include
  for f in list_sources
    incs += extract_includes f
  end
  incs = incs.sort.uniq
end

def add_includes(lines, incs)
  more = incs.collect { |i| "#include <#{i}>" }
  for m in more
    lines.insert(1, m)
    puts "#{m} added"
  end
  lines
end

def remove_includes(lines, incs)
  lines = lines.collect do |line|
    for i in incs
      if line.include? "#include <#{i}>"
        puts "#{line} removed"
        line = nil
        # p line
        break
      end
    end
    line
  end.compact
  while lines.count > 0 and lines[0].strip == ""
    lines.shift(1)
  end
  lines
end

def dump_stds
  stds = list_sources.collect { |f| extract_std_tokens f }.flatten.uniq
  d = stds.collect { |x| [x, x.split("::")[1]] }.to_h
  open("std.json", "w") { |f| f.write(JSON.pretty_generate d) }
end

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

def run(fix = false)
  std_headers = load_db
  # p std_headers

  for filename in list_sources
    # puts "checking %s" % [f]
    stds = extract_std_tokens(filename)
    for x in stds
      if not std_headers[x]
        puts "missing header info for %s" % [x]
      end
    end
    required = stds.collect { |x| std_headers[x] }.compact.sort.uniq
    # puts "%s required: %s" % [required.count, (required.join ", ")]
    incs_all = extract_includes(filename)
    # puts "%d included %s" % [incs.count, (incs.join ", ")]

    # puts "%s current: %s" % [incs.count, (incs.join ", ")]

    incs = incs_all
      .select { |i| not i =~ /\.h/ }
      .select { |i| not i =~ /\.hpp/ }
      .select { |i| not i =~ /ttl\// }
      .select { |i| not i =~ /stdml\// }
      .select { |i| not i =~ /tracer\// }
      .select { |i| not i =~ /^go$/ }

    redundent = incs - required
    missing = required - incs

    if redundent.count > 0 or missing.count > 0
      puts "#{filename}"
      # puts "%s incs_all: %s" % [incs_all.count, (incs_all.join ", ")]
      # puts "%s current: %s" % [incs.count, (incs.join ", ")]
      puts "%d redundent %s" % [redundent.count, (redundent.join ", ")]
      puts "%d missing %s" % [missing.count, (missing.join ", ")]

      lines = open(filename).each_line.collect { |l| l.strip }
      lines = add_includes(lines, missing)
      lines = remove_includes(lines, redundent)
      text = lines.join ("\n")
      open(filename, "w") { |f| f.write(text + "\n") } if fix
      puts
    end
  end
  `./scripts/lint/fmt.sh` if fix
end

def main
  # run
  run true
end

main
# p extract_std_tokens("include/stdml/bits/collective/session.hpp")
