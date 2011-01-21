require 'test/unit'
require 'socket'
require 'net/http'

class GetTests < Test::Unit::TestCase
    def setup
        @server = fork {
          exec 'src/spade -c config/test.cfg -p 8000'
        }
        sleep 0.1
        @http = Net::HTTP.start('localhost', 8000)
    end

    def teardown
        Process.kill("TERM", @server)
    end

    def test_404
        response = @http.get('/not/here')
        assert_equal "404", response.code
    end

    def test_static_text
      assert_same_static '/small.txt'
      assert_same_static '/large.txt'
    end

    def test_static_html
        assert_same_static '/small.html'
        assert_same_static '/large.html'
    end

    def test_static_jpg
        assert_same_static '/small.gif'
        assert_same_static '/large.jpg'
    end

    def test_dynamic
        assert_same_dynamic '/adder?1&2', "3"
        assert_same_dynamic '/adder?', ""
    end

    def assert_same_static path, filename=nil
        filename ||= "tests/static#{path}"
        response = @http.get(path)
        assert_equal "200", response.code
        assert_equal File.binread(filename), response.body
    end

    def assert_same_dynamic path, expected
        response = @http.get(path)
        assert_equal "200", response.code
        assert_equal expected, response.body.strip
    end
end
