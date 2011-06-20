# Find pizza in New York using Google Local

page = new WebPage()

page.open 'http://www.google.com/m/local?site=local&q=pizza+in+new+york',
  (status) ->
    if status isnt 'success'
      console.log 'Unable to access network'
    else
      results = page.evaluate ->
        pizza = []
        list = document.querySelectorAll 'div.bf'
        for item in list
          pizza.push(item.innerText)
        return pizza
      console.log results.join('\n')
    phantom.exit()
