var t = 10;
while (t > 0) {
    console.log(t);
    phantom.sleep(1000);
    t = t - 1;
}
console.log('BLAST OFF');
phantom.exit();
