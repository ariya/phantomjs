//This code will read all the files with extension html on the current directory, 
//loading for each of them an external common CSS file.

var fs = require('fs');
var current_dir = fs.absolute(".");
console.log("The tables HTML files are in: " + current_dir);
var css_file = fs.absolute("../css/") + "main.css";
console.log("The CSS main file is in: " + css_file);

var path = current_dir;
// Get a list all files in directory
var list = fs.list(path);

function render_pages(){
    var pages=[];
    for(var x = 0, n = 0; x < list.length; x++){
      // Note: If you didn't end path with a slash, you need to do so here.
        var file_name = list[x];
        var file_full_path = path + file_name;
        
        //it must be a file with the format of XX.html
        if(fs.isFile(file_full_path) && (file_name.split("."))[1]=="html" ){
            console.log("Creating page from "+file_name);
           
            pages[n] = require('webpage').create();
            
            //console.log(x+"  "+file_name);

            pages[n].settings.localToRemoteUrlAccessEnabled = true;

            var content = '';
            content += '<html><head>';
            content += '<link rel="stylesheet" href="file://'+ css_file + '" type="text/css" media="screen">';
            content += '</head><body>';
            content += fs.read(path + file_name);
            content += '</body></html>';

            pages[n].content = content;
            
            var img_fname = (file_name.split("."))[0]+".png";
            console.log('Rendering file ' + current_dir+img_fname);
            pages[n].render(current_dir+img_fname);

            pages[n].close();
            
            n++;
        }
    }
}


//upload common CSS file for caching
var page_css = require('webpage').create();
var content_css = '<html><head><link rel="stylesheet" href="file://'+ css_file + '" type="text/css" media="screen"></head><body></body></html>';
page_css.content = content_css;
page_css.onLoadFinished = function(status) {
  console.log('CSS status file: ' + status);
  render_pages();
  phantom.exit();
};
