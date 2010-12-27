var t = 10;
while (t > 0) {
    phantom.log(t);
    phantom.sleep(1000);
    t = t - 1;
}
phantom.log('BLAST OFF');
phantom.exit();
