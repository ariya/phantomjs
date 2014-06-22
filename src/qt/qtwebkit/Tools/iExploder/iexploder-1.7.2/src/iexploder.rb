# encoding: ASCII-8BIT

# iExploder - Generates bad HTML files to perform QA for web browsers.
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

require 'cgi'
require 'yaml'

require './scanner.rb'
require './version.rb'

# Used to speed up subtest generation
$TEST_CACHE = {}

# Media extensions to proper mime type map (not that we always listen'
$MIME_MAP = {
  'bmp' => 'image/bmp',
  'gif' => 'image/gif',
  'jpg' => 'image/jpeg',
  'png' => 'image/png',
  'svg' => 'image/svg+xml',
  'tiff' => 'image/tiff',
  'xbm' => 'image/xbm',
  'ico' => 'image/x-icon',
  'jng' => 'image/x-jng',
  'xpm' => 'image/x-portable-pixmap',
  'ogg' => 'audio/ogg',
  'snd' => 'audio/basic',
  'wav' => 'audio/wav'
}

# These tags get src properties more often than others
$SRC_TAGS = ['img', 'audio', 'video', 'embed']

class IExploder
  attr_accessor :test_num, :subtest_data, :lookup_mode, :random_mode, :cgi_url, :browser, :claimed_browser
  attr_accessor :offset, :lines, :stop_num, :config

  def initialize(config_path)
    @config = YAML::load(File.open(config_path))
    @stop_num = nil
    @subtest_data = nil
    @test_num = 0
    @cgi_url = '/iexploder.cgi'
    @browser = 'UNKNOWN'
    @claimed_browser = nil
    readTagFiles()
    return nil
  end

  def setRandomSeed
    if @test_num > 0
      srand(@test_num)
    else
      srand
    end
  end


  def readTagFiles
    # These if statements are so that mod_ruby doesn't have to reload the files
    # each time
    data_path = @config['mangle_data_path']
    @cssTags = readTagsDir("#{data_path}/css-properties")
    @cssPseudoTags = readTagsDir("#{data_path}/css-pseudo")
    @cssAtRules = readTagsDir("#{data_path}/css-atrules")
    @htmlTags = readTagsDir("#{data_path}/html-tags")
    @htmlAttr = readTagsDir("#{data_path}/html-attrs")
    @htmlValues = readTagsDir("#{data_path}/html-values")
    @cssValues = readTagsDir("#{data_path}/css-values")
    @headerValues = readTagsDir("#{data_path}/headers")
    @protocolValues = readTagsDir("#{data_path}/protocols")
    @mimeTypes = readTagsDir("#{data_path}/mime-types")
    @media = readMediaDir("#{data_path}/media")
  end

  def readTagsDir(directory)
    values = []
    Dir.foreach(directory) { |filename|
      if File.file?(directory + "/" + filename)
        values = values + readTagFile(directory + "/" + filename)
      end
    }
    return values.uniq
  end

  def readMediaDir(directory)
    data = {}
    Dir.foreach(directory) { |filename|
      if File.file?(directory + "/" + filename)
       (base, extension) = filename.split('.')
        mime_type = $MIME_MAP[extension]
        data[mime_type] = File.read(directory + "/" + filename)
      end
    }
    return data
  end

  def readTagFile(filename)
    list = Array.new
    File.new(filename).readlines.each { |line|
      line.chop!

      # Don't include comments.
      if (line !~ /^# /) && (line.length > 0)
        list << line
      end
    }
    return list
  end


  def generateHtmlValue(tag)
    choice = rand(100)
    tag = tag.sub('EXCLUDED_', '')
    if tag =~ /^on/ and choice < 90
      return generateHtmlValue('') + "()"
    elsif tag == 'src' or tag == 'data' or tag == 'profile' and choice < 90
      return generateGarbageUrl(tag)
    end

    case choice
      when 0..50 then
        return @htmlValues[rand(@htmlValues.length)]
      when 51..75
        return generateGarbageNumber()
      when 76..85
        return generateGarbageValue()
      when 86..90
        return generateGarbageNumber() + ',' + generateGarbageNumber()
      when 91..98
        return generateGarbageUrl(tag)
    else
      return generateOverflow()
    end
  end

  def generateMediaUrl(tag)
    mime_type = @media.keys[rand(@media.keys.length)]
    return generateTestUrl(@test_num, nil, nil, mime_type)
  end

  def generateGarbageUrl(tag)
    choice = rand(100)
    case choice
      when 0..30
      return generateMediaUrl(tag)
      when 31..50
      return @protocolValues[rand(@protocolValues.length)] + '%' + generateGarbageValue()
      when 51..60
      return @protocolValues[rand(@protocolValues.length)] + '//../' + generateGarbageValue()
      when 60..75
      return @protocolValues[rand(@protocolValues.length)] + '//' + generateGarbageValue()
      when 75..85
      return generateOverflow() + ":" + generateGarbageValue()
      when 86..97
      return generateGarbageValue() + ":" + generateOverflow()
    else
      return generateOverflow()
    end
  end

  def generateCssValue(property)
    size_types = ['', 'em', 'px', '%', 'pt', 'pc', 'ex', 'in', 'cm', 'mm']

    choice = rand(100)
    case choice
      when 0..50 then
      # return the most likely scenario
      case property.sub('EXCLUDED_', '')
        when /-image|content/
          return 'url(' + generateGarbageUrl(property) + ')'
        when /-width|-radius|-spacing|margin|padding|height/
          return generateGarbageValue() + size_types[rand(size_types.length)]
        when /-color/
          return generateGarbageColor()
        when /-delay|-duration/
          return generateGarbageValue() + 'ms'
      else
        return @cssValues[rand(@cssValues.length)]
      end
      when 51..75 then return generateGarbageNumber()
      when 76..85 then return 'url(' + generateGarbageUrl(property) + ')'
      when 85..98 then return generateGarbageValue()
    else
      return generateOverflow()
    end
  end

  def generateGarbageColor()
    case rand(100)
      when 0..50 then return '#' + generateGarbageValue()
      when 51..70 then return 'rgb(' + generateGarbageNumber() + ',' + generateGarbageNumber() + ',' + generateGarbageNumber() + ')'
      when 71..98 then return 'rgb(' + generateGarbageNumber() + '%,' + generateGarbageNumber() + '%,' + generateGarbageNumber() + '%)'
    else
      return generateOverflow()
    end
  end

  def generateGarbageNumber()
    choice = rand(100)
    case choice
      when 0 then return '0'
      when 1..40 then return '9' * rand(100)
      when 41..60 then return '999999.' + rand(999999999999999999999).to_s
      when 61..80 then return '-' + ('9' * rand(100))
      when 81..90 then return '-999999.' + rand(999999999999999999999).to_s
      when 91..98 then return generateGarbageText()
    else
      return generateOverflow()
    end
  end

  def generateGarbageValue()
    case rand(100)
      when 0..30 then return rand(255).chr * rand(@config['buffer_overflow_length'])
      when 31..50 then return "%n" * 50
      when 51..65 then return ("&#" + rand(999999).to_s + ";") * rand(@config['max_garbage_text_size'])
      when 66..70 then
      junk = []
      0.upto(rand(20)+1) do
        junk << "\\x" + rand(65535).to_s(16)
      end
      return junk.join('') * rand(@config['max_garbage_text_size'])
      when 71..99 then
      junk = []
      chars = '%?!$#^0123456789ABCDEF%#./\&|;'
      0.upto(rand(20)+1) do
        junk << chars[rand(chars.length)].chr
      end
      return junk.join('') * rand(@config['max_garbage_text_size'])
    end
  end

  def generateOverflow()
    return rand(255).chr * (@config['buffer_overflow_length'] + rand(500))
  end

  def generateGarbageText
    case rand(100)
      when 0..70 then return 'X' * 129
      when 71..75 then return "%n" * 15
      when 76..85 then return ("&#" + rand(9999999999999).to_s + ";") * rand(@config['max_garbage_text_size'])
      when 86..90 then return generateGarbageValue()
      when 91..98 then return rand(255).chr * rand(@config['max_garbage_text_size'])
    else
      return generateOverflow()
    end
  end

  def isPropertyInBlacklist(properties)
    # Format: [img, src] or [img, style, property]
    blacklist_entries = []
    if @config.has_key?('exclude') and @config['exclude']
      blacklist_entries << properties.join('.')
      wildcard_property = properties.dup
      wildcard_property[0] = '*'
      blacklist_entries << wildcard_property.join('.')
      blacklist_entries.each do |entry|
        if @config['exclude'].has_key?(entry) and @browser =~ /#{@config['exclude'][entry]}/
          return true
        end
      end
    end
    return false
  end

  def generateCssStyling(tag)
    out = ' style="'
    0.upto(rand(@config['properties_per_style_max'])) {
      property = @cssTags[rand(@cssTags.length)]
      if isPropertyInBlacklist([tag, 'style', property])
        property = "EXCLUDED_#{property}"
      end
      out << property

      # very small chance we let the tag run on.
      if rand(65) > 1
        out << ": "
      end

      values = []
      0.upto(rand(@config['attributes_per_style_property_max'])) {
        values << generateCssValue(property)
      }
      out << values.join(' ')
      # we almost always put the ; there.
      if rand(65) > 1
        out << ";\n    "
      end
    }
    out << "\""
    return out
  end

  def mangleTag(tag, no_close_chance=false)
    if not no_close_chance and rand(100) < 15
      return "</" + tag + ">"
    end
    out = "<" + tag
    if rand(100) > 1
      out << ' '
    else
      out << generateOverflow()
    end

    attrNum = rand(@config['attributes_per_html_tag_max']) + 1
    attrs = []
    # The HTML head tag does not have many useful attributes, but is always included in tests.
    if tag == 'head' and rand(100) < 75
      case rand(3)
        when 0 then attrs << 'lang'
        when 1 then attrs << 'dir'
        when 2 then attrs << 'profile'
      end
    end
    # 75% of the time, these tags get a src attribute
    if $SRC_TAGS.include?(tag) and rand(100) < 75
      if @config.has_key?('exclude') and @config['exclude'] and @config['exclude'].has_key?("#{tag}.src")
        attrs << 'EXCLUDED_src'
      else
        attrs << 'src'
      end
    end

    while attrs.length < attrNum
      attribute = @htmlAttr[rand(@htmlAttr.length)]
      if isPropertyInBlacklist([tag, attribute])
        attribute = "EXCLUDED_#{attribute}"
      end
      attrs << attribute
    end

    # Add a few HTML attributes
    for attr in attrs
      out << attr
      if rand(100) > 1
        out << '='
      end
      if (rand(100) >= 50)
        quoted = 1
        out << "\""
      else
        quoted = nil
      end
      out << generateHtmlValue(attr)
      if quoted
        if rand(100) >= 10
          out << "\""
        end
      end
      if rand(100) >= 1
        out << "\n  "
      end
    end

    if rand(100) >= 25
      out << generateCssStyling(tag)
    end
    out << ">\n"
    return out
  end

  def nextTestNum()
    if @subtest_data
      return @test_num
    elsif @random_mode
      return rand(99999999999)
    else
      return @test_num  + 1
    end
  end

  def generateCssPattern()
    # Generate a CSS selector pattern.
    choice = rand(100)
    pattern = ''
    case choice
      when 0..84 then pattern = @htmlTags[rand(@htmlTags.length)].dup
      when 85..89 then pattern = "*"
      when 90..94 then pattern = @cssAtRules[rand(@cssAtRules.length)].dup
      when 95..100 then pattern = ''
    end

    if rand(100) < 25
      pattern << " " + @htmlTags[rand(@htmlTags.length)]
    end

    if rand(100) < 25
      pattern << " > " + @htmlTags[rand(@htmlTags.length)]
    end

    if rand(100) < 25
      pattern << " + " + @htmlTags[rand(@htmlTags.length)]
    end

    if rand(100) < 10
      pattern << "*"
    end


    if rand(100) < 25
      pseudo = @cssPseudoTags[rand(@cssPseudoTags.length)].dup
      # These tags typically have a parenthesis
      if (pseudo =~ /^lang|^nth|^not/ and rand(100) < 75 and pseudo !~ /\(/) or rand(100) < 20
        pseudo << '('
      end

      if pseudo =~ /\(/
        if rand(100) < 75
          pseudo << generateGarbageValue()
        end
        if rand(100) < 75
          pseudo << ')'
        end
      end
      pattern << ":" + pseudo
    end

    if rand(100) < 20
      html_attr = @htmlAttr[rand(@htmlAttr.length)]
      match = '[' + html_attr
      choice = rand(100)
      garbage = generateGarbageValue()
      case choice
        when 0..25 then match << ']'
        when 26..50 then match << "=\"#{garbage}\"]"
        when 51..75 then match << "=~\"#{garbage}\"]"
        when 76..99 then match << "|=\"#{garbage}\"]"
      end
      pattern << match
    end

    if rand(100) < 20
      if rand(100) < 50
        pattern << '.' + generateGarbageValue()
      else
        pattern << '.*'
      end
    end

    if rand(100) < 20
      pattern << '#' + generateGarbageValue()
    end

    if rand(100) < 5
      pattern << ' #' + generateGarbageValue()
    end

    return pattern
  end

  def buildStyleTag()
    out = "\n"
    0.upto(rand(@config['properties_per_style_max'])) {
      out << generateCssPattern()
      if rand(100) < 90
        out << " {\n"
      end

      0.upto(rand(@config['properties_per_style_max'])) {
        property = @cssTags[rand(@cssTags.length)].dup
        if isPropertyInBlacklist(['style', 'style', property])
          property = "  EXCLUDED_#{property}"
        end
        out << "  #{property}: "

        values = []
        0.upto(rand(@config['attributes_per_style_property_max'])) {
          values << generateCssValue(property)
        }
        out << values.join(' ')
        if rand(100) < 95
          out << ";\n"
        end
      }
      if rand(100) < 90
        out << "\n}\n"
      end

    }
    return out
  end


  # Build any malicious javascript here. Fairly naive at the moment.
  def buildJavaScript
    target = @htmlTags[rand(@htmlTags.length)]
    css_property = @cssTags[rand(@cssTags.length)]
    css_property2 = @cssTags[rand(@cssTags.length)]
    html_attr = @htmlAttr[rand(@htmlAttr.length)]
    css_value = generateCssValue(css_property)
    html_value = generateHtmlValue(html_attr)
    html_value2 = generateGarbageNumber()
    mangled = mangleTag(@htmlTags[rand(@htmlTags.length)]);
    mangled2 = mangleTag(@htmlTags[rand(@htmlTags.length)]);

    js = []
    js << "window.onload=function(){"
    js << "  var ietarget = document.createElement('#{target}');"
    js << "  ietarget.style.#{css_property} = '#{css_value}';"
    js << "  ietarget.#{html_attr} = '#{html_value}';"
    js << "  document.body.appendChild(ietarget);"
    js << "  ietarget.style.#{css_property2} = #{html_value2};"

    js << "  document.write('#{mangled}');"
    js << "  document.write('#{mangled2}');"
    js << "}"
    return js.join("\n")
  end

  def buildMediaFile(mime_type)
    if @media.has_key?(mime_type)
      data = @media[mime_type].dup
    else
      puts "No media found for #{mime_type}"
      data = generateGarbageText()
    end

    # corrupt it in a subtle way
    choice = rand(100)
    if choice > 50
      garbage = generateGarbageValue()
    else
      garbage = rand(255).chr * rand(8)
    end

    if "1.9".respond_to?(:encoding)
      garbage.force_encoding('ASCII-8BIT')
      data.force_encoding('ASCII-8BIT')
    end

    garbage_start = rand(data.length)
    garbage_end = garbage_start + garbage.length
    data[garbage_start..garbage_end] = garbage
    if rand(100) < 15
      data << generateGarbageValue()
    end
    return data
  end

  # Parse the subtest data passed in as part of the URL
  def parseSubTestData(subtest_data)
    # Initialize with one line at 0
    if not subtest_data or subtest_data.to_i == 0
      return [@config['initial_subtest_width'], [0]]
    end
     (lines_at_time, offsets_string) = subtest_data.split('_')
    offsets = offsets_string.split(',').map! {|x| x.to_i }
    return [lines_at_time.to_i, offsets]
  end

  def generateTestUrl(test_num, subtest_width=nil, subtest_offsets=nil, mime_type=nil)
    url = @cgi_url + '?'
    if subtest_width
      if subtest_offsets.length > @config['subtest_combinations_max']
        url << "t=" << test_num.to_s << "&l=test_redirect&z=THE_END"
      else
        url << "t=" << test_num.to_s << "&s=" << subtest_width.to_s << "_" << subtest_offsets.join(',')
      end
    else
      url << "t=" << test_num.to_s
    end

    if @random_mode
      url << "&r=1"
    elsif @stop_num
      url << "&x=" << @stop_num.to_s
    end

    if mime_type
      url << '&m=' + CGI::escape(mime_type)
    end

    url << "&b=" << CGI::escape(@browser)
    return url
  end

  def buildBodyTags(tag_count)
    tagList = ['body']
    # subtract the <body> tag from tag_count.
    1.upto(tag_count-1) { tagList << @htmlTags[rand(@htmlTags.length)] }

    # Lean ourselves toward lots of img and src tests
    for tag, percent in @config['favor_html_tags']
      if rand(100) < percent.to_f
        # Don't overwrite the body tag.
        tagList[rand(tagList.length-1)+1] = tag
      end
    end

    # Now we have our hitlist of tags,lets mangle them.
    mangled_tags = []
    tagList.each do |tag|
      tag_data = mangleTag(tag)
      if tag == 'script'
        if rand(100) < 40
          tag_data = "<script>"
        end
        tag_data << buildJavaScript() + "\n" + "</script>\n"
      elsif tag == 'style'
        if rand(100) < 40
          tag_data = "<style>"
        end
        tag_data << buildStyleTag() + "\n" + "</style>\n"
      elsif rand(100) <= 90
        tag_data << generateGarbageText() << "\n"
      else
        tag_data << "\n"
      end

      if rand(100) <= 33
        tag_data << "</#{tag}>\n"
      end
      mangled_tags << "\n<!-- START #{tag} -->\n" + tag_data + "\n<!-- END #{tag} -->\n"
    end
    return mangled_tags
  end

  def buildHeaderTags(tag_count)
    valid_head_tags = ['title', 'base', 'link', 'meta']
    header_tags = ['html', 'head']
    1.upto(tag_count-1) { header_tags << valid_head_tags[rand(valid_head_tags.length)] }
    header_tags << @htmlTags[rand(@htmlTags.length)]
    mangled_tags = []
    header_tags.each do |tag|
      mangled_tags << mangleTag(tag, no_close_chance=true)
    end
    return mangled_tags
  end

  def buildSurvivedPage(page_type)
    page = "<html><head>"
    page << "<body>Bummer. You survived both redirects. Let me go sulk in the corner.</body>"
    page << "</html>"
    return page
  end

  def buildRedirect(test_num, subtest_data, lookup_mode, stop_num=nil)
    # no more redirects.
    if lookup_mode == '1' or stop_num == test_num
      return ''
    end

    if subtest_data
      width, offsets = parseSubTestData(@subtest_data)
    else
      width, offsets = nil
    end

    # We still need a redirect, but don't bother generating new data.
    if lookup_mode
      redirect_url = generateTestUrl(test_num, width, offsets)
      if lookup_mode == 'test_redirect'
        redirect_url << "&l=test_another_redirect"
      elsif lookup_mode == 'test_another_redirect'
        redirect_url << "&l=survived_redirect"
      else
        redirect_url << "&l=#{lookup_mode}"
      end
    else
      # This is a normal redirect going on to the next page. If we have subtest, get the next one.
      if subtest_data
        width, offsets = combine_combo_creator(@config['html_tags_per_page'], width, offsets)[0..1]
      end
      redirect_url = generateTestUrl(nextTestNum(), width, offsets)
    end

    redirect_code = "\t<META HTTP-EQUIV=\"Refresh\" content=\"0;URL=#{redirect_url}\">\n"
    # use both techniques, because you never know how you might be corrupting yourself.
    redirect_code << "\t<script language=\"javascript\">setTimeout('window.location=\"#{redirect_url}\"', 1000);</script>\n"
    return redirect_code
  end

  def buildPage()
    if @lookup_mode == 'survived_redirect'
      return self.buildSurvivedPage(@lookup_mode)
    end
    tag_count = @config['html_tags_per_page']

    if $TEST_CACHE.has_key?(@test_num)
     (header_tags, body_tags) = $TEST_CACHE[@test_num]
    else
      header_tags = buildHeaderTags(3)
      body_tags = buildBodyTags(tag_count - header_tags.length)
    end
    required_tags = {
      0 => 'html',
      1 => 'head',
      header_tags.length => 'body'
    }

    if @subtest_data and @subtest_data.length > 0
      if not $TEST_CACHE.has_key?(@test_num)
        $TEST_CACHE[@test_num] = [header_tags, body_tags]
      end
      (width, offsets) = parseSubTestData(@subtest_data)
      lines = combine_combo_creator(tag_count, width, offsets)[2]
      all_tags = header_tags + body_tags
      body_start = header_tags.length
      header_tags = []
      body_tags = []
      # <html> and <body> are required, regardless of their existence in the subtest data.
      0.upto(tag_count) do |line_number|
        tag_data = nil
        if lines.include?(line_number)
          tag_data = all_tags[line_number]
        elsif required_tags.key?(line_number)
          tag_data = "<" + required_tags[line_number] + ">"
        end
        if tag_data
          if line_number < body_start
            header_tags << tag_data
          else
            body_tags << tag_data
          end
        end
      end
      header_tags << "<!-- subtest mode: #{offsets.length} combinations, width: #{width} -->"
    end

    htmlText = header_tags[0..1].join("\n\t")
    htmlText << buildRedirect(@test_num, @subtest_data, @lookup_mode, @stop_num)
    htmlText << "<title>[#{@test_num}:#{@subtest_data}] iExploder #{$VERSION} - #{generateGarbageText()}</title>\n"
    if @claimed_browser and @claimed_browser.length > 1
      show_browser = @claimed_browser
    else
      show_browser = @browser
    end
    htmlText << "\n<!-- iExploder #{$VERSION} | test #{@test_num}:#{@subtest_data} at #{Time.now} -->\n"
    htmlText << "<!-- browser: #{show_browser} -->\n"
    htmlText << header_tags[2..-1].join("\n\t")
    htmlText << "\n</head>\n\n"
    htmlText << body_tags.join("\n")
    htmlText << "</body>\n</html>"
    return htmlText
  end

  def buildHeaders(mime_type)
    use_headers = []
    banned_headers = []
    response = {'Content-Type' => mime_type}
    0.upto(rand(@config['headers_per_page_max'])) do
      try_header = @headerValues[rand(@headerValues.length)]
      if ! banned_headers.include?(try_header.downcase)
        use_headers << try_header
      end
    end
    for header in use_headers.uniq
      if rand(100) > 75
        response[header] = generateGarbageNumber()
      else
        response[header] = generateGarbageUrl(header)
      end
    end
    return response
  end
end


# for testing
if $0 == __FILE__
  ie = IExploder.new('config.yaml')
  ie.test_num = ARGV[0].to_i || 1
  ie.subtest_data = ARGV[1] || nil
  mime_type = ARGV[2] || nil
  ie.setRandomSeed()
  if not mime_type
    html_output = ie.buildPage()
    puts html_output
  else
    headers = ie.buildHeaders(mime_type)
    for (key, value) in headers
      puts "#{key}: #{value}"
    end
    puts "Mime-Type: #{mime_type}"
    puts ie.buildMediaFile(mime_type)
  end
end
