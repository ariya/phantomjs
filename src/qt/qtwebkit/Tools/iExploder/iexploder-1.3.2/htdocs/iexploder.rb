# iExploder - Generates bad HTML files to perform QA for web browsers.
# Developed for the Mozilla Foundation.
#####################
#
# Copyright (c) 2006 Thomas Stromberg <thomas%stromberg.org>
# 
# This software is provided 'as-is', without any express or implied warranty.
# In no event will the authors be held liable for any damages arising from the
# use of this software.
# 
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software in a
# product, an acknowledgment in the product documentation would be appreciated
# but is not required.
# 
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
# 
# 3. This notice may not be removed or altered from any source distribution.

$VERSION="1.3.2"

class IExploder
    attr_accessor :test_num, :subtest_num, :lookup_mode, :random_mode, :url
    attr_accessor :offset, :lines, :stop_num
    
    def initialize(max_tags, max_attrs, max_props)
        @htmlMaxTags = max_tags
        @htmlMaxAttrs = max_attrs
        @cssMaxProps = max_props
        @mangledTagTotal = 0
        @stop_num = 0
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
        
        if (! @cssTags)
            @cssTags = readTagFile('cssproperties.in');
        end
        
        if (! @htmlTags)
            @htmlTags = readTagFile('htmltags.in');
        end
        if (! @htmlAttr)
            @htmlAttr = readTagFile('htmlattrs.in');
        end
        
        if (! @htmlValues)
            @htmlValues = readTagFile('htmlvalues.in');
        end
        
        if (! @cssValues)
            @cssValues = readTagFile('cssvalues.in');
        end
        
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
        return  list
    end
    
    # based on make_up_value, essentially.
    def inventValue
        value = rand(19);
        case value
        when 1..3 then return (@htmlValues[rand(@htmlValues.length)])
        when 4..5 then return (@htmlValues[rand(@htmlValues.length)] + inventValue())
        when 6 then return (@htmlValues[rand(@htmlValues.length)] + "//" + inventValue())
        when 7 then return ''
            # this may return negative argument?
        when 8..10 then return rand(255).chr * (rand(256)+8)
        when 11 then return rand(255).chr * (rand(2048)+8)
        when 12 then return "#" + rand(999999).to_s
        when 13 then return rand(999999).to_s + "%"
        when 14..15 then return "&" + rand(999999).to_s + ";"
            # filters
        when 16 then
            return inventValue() + "=" + inventValue()
            
            # this my return undefined method + for nil:NilClass
        when 17 then return inventValue() + "," + inventValue()
        else
            if rand(5) > 3
                return "-" + rand(999999).to_s
            else
                return rand(999999).to_s
            end
        end
    end
    
    # based on make_up_value, essentially.
    def inventCssValue(tag)
        value = rand(23);
        case value
        when 1..10 then return @cssValues[rand(@cssValues.length)]
        when 11 then return ''
        when 12 then return rand(255).chr * (rand(8192)+8)
        when 13
            length = rand(1024) + 8
            return (rand(255).chr * length) + " " + (rand(255).chr * length) + " " + (rand(255).chr * length)
        when 14 then return (rand(255).chr * (rand(1024)+3)) + "px"
        when 15 then return (rand(255).chr * (rand(1024)+3)) + "em"
        when 16 then return "url(" + inventValue() + ")"
        when 17..18 then return "#" + rand(999999999).to_s
        when 19 then return "-" + rand(99999999).to_s
        else return rand(99999999).to_s;
        end
    end
    
    
    def mangleTag(tag)
        @mangledTagTotal += 1
        out = ''
        
        # 20% chance of closing a tag instead of opening it. This 
        # still counts against @mangledTagTotal, however.
        if rand(10) > 8
            out = "</" + tag + ">"
            return out
        end
        
        # we're opening it.
        out = "<" + tag
        
        # forgot the space between the tag and the attributes
        if rand(15) > 1
            out << ' '
        end
        
        attrNum = rand(@htmlMaxAttrs) + 1
        
        1.upto(attrNum) {
            attr = @htmlAttr[rand(@htmlAttr.length)]
            
            out << attr
            
            # 7.5% of the time we skip the = sign. Don't prefix it
            # if the attribute ends with a ( however.
            
            
            if rand(15) > 1
                out << '='
            end
            
            # sometimes quote it, sometimes not. I doubt the importance
            # of this test, but mangleme-1.2 added it, and adding more
            # random-ness never hurt anything but time. I'll do it less often.
            quote = rand(2)
            if (quote > 1)
                out << "\""
            end
            
            out << inventValue()
            
            # end the quote when you are done
            if (quote > 1)
                out << "\" "
            end
            
            # 5% chance we skip the space at the end of the name
            if rand(20) > 1
                out << ' '
            end
            
        }
        
        # CSS styles!
        if rand(4) > 1
            out << " style=\""
            1.upto(rand(@cssMaxProps)+1) {
                out << @cssTags[rand(@cssTags.length)]
                
                # very small chance we let the tag run on.
                if rand(50) > 1
                    out << ": "
                end
                
                out << inventCssValue(tag)
                # we almost always put the ; there.
                if rand(50) > 1
                    out << '; '
                end
            }
            out << "\""
        end
        
        out << ">\n"
        
        # support our local troops!
        if (@subtest_num > 0) && filterSubTest() 
            if tag =~ /html|body|head/
                return '<' + tag + '>'
            else
                return "<x-#@mangledTagTotal>\n"
            end
        else
            return out
        end    
    end
    #end
    
    def filterSubTest()
        result = 1
        if (@mangledTagTotal >= @offset) && (@mangledTagTotal < (@offset + @lines))
            result = nil
        end
        return result
    end
    
    def nextTestNum()
        if random_mode
            n = rand(99999999)
        else
            if @test_num
                n = @test_num  + 1
            else
                n = 1
            end
        end
        return n
    end
    
    # If we are at line 30 with 8 extra lines, there is no point to try line 31
    # with 8 lines as well.. skip back to 1 and bump up the line count.
    def nextSubTestNum()    
        if (@offset + @lines) > @htmlMaxTags
            nextNum = ((@lines * 2 -1)) * @htmlMaxTags
        else
            nextNum = @subtest_num + 1
        end      
        return nextNum
    end
    
    
    def buildPage
        if (! @test_num) || (@test_num < 1)
            @test_num = 1
        end
        next_num=nextTestNum()
        @lines = @subtest_num.div(@htmlMaxTags) + 1
        @offset = @subtest_num.modulo(@htmlMaxTags)
        
        # building the HTML
        bodyText = mangleTag('html')
        bodyText << "\n<head>\n"
        
        # Only do redirects if lookup=1 has not been specified.
        if (! @lookup_mode) && (@lines <= @htmlMaxTags) && (@stop_num != @test_num)
            newpage = @url + "?"
            if @subtest_num > 0
                newpage << "test=" << @test_num.to_s << "&subtest=" << nextSubTestNum().to_s
            else
                newpage << "test=" << next_num.to_s
            end
            
            if @random_mode
                newpage << "&random=1"
            end
            
            if @stop_num > 0
                newpage << "&stop=" << @stop_num.to_s
            end
            
            bodyText << "\t<META HTTP-EQUIV=\"Refresh\" content=\"0;URL=#{newpage}\">\n" 
            # use both techniques, because you never know how you might be corrupting yourself.
            bodyText << "\t<script language=\"javascript\">setTimeout('window.location=\"#{newpage}\"', 1000);</script>\n" 
        end
        
        bodyText << "\t" << mangleTag('meta')
        bodyText << "\t" <<  mangleTag('meta')
        bodyText << "\t" <<  mangleTag('link')
        
        bodyText << "\t<title>[#@test_num] iExploder #{$VERSION} - #{inventValue()}</title>\n"
        bodyText << "</head>\n\n"
        
        # What tags will we be messing with ######################
        tagList = [ 'body']
        
        # we already have 5 tags?
        1.upto(@htmlMaxTags - 5 ) { tagList << @htmlTags[rand(@htmlTags.length)] }
        
        tagList.each { |tag|
            bodyText << mangleTag(tag)
            bodyText << inventValue() + "\n"
        }
        bodyText << "</body>\n</html>"
    end
end



if $0 == __FILE__
    max=ARGV[0].to_i
    puts "testing #{max} tags"
    test = IExploder.new(max, 5, 5)
    test.readTagFiles()
    test.test_num=1
    test.subtest_num=1
    counter=0
    test.lines=0

    while test.lines < max
        test.lines = test.subtest_num.div(max) + 1
        test.offset = test.subtest_num.modulo(max)
        test.subtest_num=test.nextSubTestNum
        counter = counter + 1
	puts "[#{counter}] subtest #{test.subtest_num} is #{test.lines} lines with #{test.offset} offset"
    end

    puts "for #{max} tests, you will have #{counter} iterations until #{test.subtest_num}"
end

