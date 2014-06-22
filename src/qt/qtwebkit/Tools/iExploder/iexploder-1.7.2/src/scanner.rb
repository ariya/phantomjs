#
# iExploder Combination Scanner Library (used for subtesting)
#
# Copyright 2010 Thomas Stromberg - All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# This is a simple sequential combination creator with a constantly growing width
def seq_combo_creator(total_lines, width, offset)
  # Offsets start at 0 to total_lines-1
  use_lines = []
  offset.upto(offset+width-1) do |line_number|
    use_lines << line_number
  end

  if use_lines[-1] == total_lines-1
    width += 1
    next_offset = 0
  else
    next_offset = offset + 1
  end
  return [width, next_offset, use_lines]
end

# This tries all combinations, giving a small test-case, but requires lots of
# subtests.
def combine_combo_creator(total_lines, width, offsets)
  #  puts "Asked: Total Lines: #{total_lines} Line Count: #{width} Offsets: #{offsets.join(',')}"
  if not offsets or offsets.length == 0
    #    puts "  Setting offsets to 0"
    offsets = [0,]
  end
  if width < 1
    width = 1
  end

  index = 0
  lines = []
  new_offsets = []
  reset_next_offset = false

  # Reverse the order from fastest to slowest
  offsets.each_with_index do |offset, index|
    0.upto(width-1) do |line_offset|
      lines << (offset + line_offset)
    end
    if lines[-1] >= total_lines - 1
      # If we are the slowest, that means we are done with this iteration.
      if index == offsets.length - 1
        new_offset_count = offsets.length + 1
        # Loosely follow the Fibonacci sequence when calculating width
        width = (new_offset_count * 1.61803398).round
        new_offsets = []
        # 0 to offsets.length creates one additional offset
        0.upto(offsets.length) do |new_offset_num|
          new_offsets << (new_offset_num * width)
        end

        # We need the lowest offset first. Oops.
        new_offsets.reverse!
      else
        # Move to the next available slot.. next offset will take the one before.
        new_offsets << offsets[index+1] + (width * 2)
        reset_next_offset = true
      end
    elsif reset_next_offset
      new_offsets << (offset + width)
      reset_next_offset = false
      # The first one always rotates
    elsif index == 0
      new_offsets << (offset + width)
      reset_next_offset = false
      # The others stay still normally.
    else
      new_offsets << offset
      reset_next_offset = false
    end
  end

  return [width, new_offsets, lines]
end

def avg(list)
  sum = list.inject(0.0) { |sum, el| sum += el }
  return sum / list.length
end


# for testing #################################################################
if $0 == __FILE__
  line_count = ARGV[0].to_i || 100
  try_count = ARGV[1].to_i || 10

  seq_iterations = []
  combine_iterations = []
  seq_size = []
  combine_size = []

  1.upto(try_count) do |run|
    puts "*" * 78
    puts "# RUN #{run} (line-count: #{line_count})"
    find_lines = []
    0.upto(rand(4)) do |count|
      choice = rand(line_count).to_i
      if ! find_lines.include?(choice)
        find_lines << choice
      end
    end

    lines = []
    width = 1
    offset = 0
    attempts = 0
    puts "Find lines: #{find_lines.join(',')}"
    while not find_lines.all? { |x| lines.include?(x) }
     (width, offset, lines) = seq_combo_creator(line_count, width, offset)
      attempts += 1
    end
    puts "Sequential found #{find_lines.join(',')} in #{attempts} attempts with #{lines.length} total lines."
    seq_iterations << attempts
    seq_size << lines.length

    lines = []
    width = 1
    offsets = []
    attempts = 0
    while not find_lines.all? { |x| lines.include?(x) }
      #    puts "Looking for #{find_lines.join(',')}"
       (width, offsets, lines) = combine_combo_creator(line_count, width, offsets)
      attempts += 1
    end
    puts "Combine found #{find_lines.join(',')} in #{attempts} attempts with #{lines.length} total lines."
    combine_iterations << attempts
    combine_size << lines.length
  end
  puts "-" * 78
  puts "Seq avg iterations=#{avg(seq_iterations).to_i} length=#{avg(seq_size).to_i}"
  puts "combine avg iterations=#{avg(combine_iterations).to_i} length=#{avg(combine_size).to_i}"
  diff_iter = (avg(combine_iterations) / avg(seq_iterations)) * 100
  diff_lines = (avg(combine_size) / avg(seq_size)) * 100
  puts "Diff iterations: #{diff_iter}%"
  puts "Diff lines: #{diff_lines}%"
end
