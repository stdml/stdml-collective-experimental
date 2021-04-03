#!/usr/bin/env ruby
require "yaml"

def include_category(pri, reg)
  {"Regex" => reg, "Priority" => pri}
end

def extract_include(filename)
  incs = []
  for line in open(filename).each_line
    if line =~ /^\#include/
      incs << line.strip
    end
  end
  incs
end

def extract_all_include
  incs = []
  roots = ["include", "src"]
  patterns = roots.collect { |r| ["#{r}/**/*.h", "#{r}/**/*.hpp", "#{r}/**/*.cpp"] }.flatten
  for f in Dir.glob(patterns)
    incs += extract_include f
  end
  incs = incs.sort.uniq
  for i in incs
    if i =~ /\<(.+)\>/
      puts "'%s'," % [$1]
    else
      puts "'%s'," % [i]
    end
  end
end

def main
  y = YAML.load open (".clang-format")

  cheaders = [
    "cassert",
    "csignal",
    "cstddef",
    "cstdint",
    "cstdio",
    "cstdlib",
    "cstring",
  ]

  cppheaders = [
    "algorithm",
    "array",
    "atomic",
    "chrono",
    "condition_variable",
    "execution",
    "experimental/buffer",
    "experimental/iterator",
    "experimental/net",
    "filesystem",
    "fstream",
    "functional",
    "future",
    "go/sync",
    "iomanip",
    "iostream",
    "iterator",
    "map",
    "memory",
    "mutex",
    "numeric",
    "optional",
    "ostream",
    "queue",
    "ranges",
    "set",
    "sstream",
    "stdexcept",
    "string",
    "system_error",
    "thread",
    "type_traits",
    "unordered_map",
    "utility",
    "vector",
  ]

  y["IncludeCategories"] = [ #
    include_category(1, "^<(%s)>$" % (cheaders.join "|")),
    include_category(1, "^<(%s)>$" % (cppheaders.join "|")),
    include_category(2, "^<stdml/"),
  ]
  open(".clang-format", "w") { |f| f.write YAML.dump (y) }
end

# extract_all_include
main
