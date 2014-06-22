if (true)
    if (true)
        alert(1);
    else if (true)
        alert(2);
    else 
        alert(3);

if (true)
    if (true) {
        alert(1)
    } else 
        alert(2);

if (true)
    if (true) {
        var a = 1;
        var b = {
            a: 1
        };
    } else 
        alert(2);

function() {
    for (var i = 0; i < 100; ++i)
        if (true)
            return true;
    return false;
}

function foo(cm) {
    if (true)
        return;
    else 
        false;
}

if (true) {
    if (false)
        if (true) {
            true;
        }
    return 2;
}

if (true) {
    if (false)
        if (true)
            true;
        else 
            false;
    else if (true)
        true;
    else 
        false;
}

if (true)
    for (; ;)
        true;
else 
    while (true)
        true;

function() {
    if (true) {
        for (; ;)
            true;
    } else if (1)
        for (; ;)
            true;
    return;
}

do 
    true;
while (true);

if (x == 1)
    alert(1);
else if (x == 2)
    alert(2);
else 
    alert(3);

// FIXME: Failing.

// if(true)try{true;}catch(e){true;}finally{true;}

