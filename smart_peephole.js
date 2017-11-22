var fs = require('fs');
var mcs = require('mcsjs');
var exec = require('child_process').exec;
var Promise = require('bluebird');
var fs = Promise.promisifyAll(require("fs"));
var deviceId = 'DEotepgV'; 
var deviceKey = 'xz1efseg7U4OjVSK';
var dataChnId = 'video';
var width = 176;
var height = 144;
var pid = 0;

var offset = 0;
var flag = 0;
var data_len;
var image_data = [];

var count = 0;
var distance = 0;
var gas = 0;
var flag = 0;

var serialport = require("serialport");
var SerialPort = serialport.SerialPort;
var serialPort = new SerialPort("/dev/ttyS0", {
    baudrate: 115200,
    parser: serialport.parsers.readline('\r\n')
});

function sleep(time, callback) {
    var stop = new Date().getTime();
    while(new Date().getTime() < stop + time) {
        ;
    }
    callback();
}

serialPort.on("open", function () {
    console.log('serial port status - opened');
});

serialPort.on('data', function(data) {
    count++;
    if (count % 3 == 1) {
        distance = data;
        myApp.emit('ultrasonic','', distance); // upload to MCS
    }
    else if (count % 3 == 2) {
        gas = data;
        myApp.emit('sensor','', gas); // upload to MCS
    }
    else {
        flag = data;
    }
    //console.log("distance, gas, flag = " + distance, gas, flag);
});


var myApp = mcs.register({
    deviceId: deviceId,
    deviceKey: deviceKey,
    host: 'api.mediatek.com'
});

exec('ffmpeg -s ' + width + 'x' + height + ' -f video4linux2 -r 30 -i /dev/video0 -f mpeg1video -r 30 -b 800k http://stream-mcs.mediatek.com/' + deviceId + '/' +deviceKey + '/' + dataChnId + '/' + width + '/' + height, function(error, stdout, stderr) {

    console.log('stdout: ' + stdout);
    console.log('stderr: ' + stderr);
    if (error !== null) {
        console.log('exec error: ' + error);    
    }
}
);


child = exec('IMAGE', function (error, stdout, stderr) {
    fs.readdir('/Media/SD-P1', function (err, data) {
        if (err) throw err;
        console.log(data);
    });

    console.log('stdout: ' + stdout);
    console.log('stderr: ' + stderr);

    if (error !== null) {
        console.log('exec error: ' + error);
    }

    myApp.on('LED', function(data, time) {       
        if (Number(data) === 1) {
            console.log('blink');
            exec("ps |grep ffmpeg | awk {'print $1}'", function (error, stdout, stderr) {
                if (error) {
                    console.error('exec error: ${error}');
                    return;
                }
                var arr = stdout.split("\n").map(function (val) { return +val; });
                pid = arr[1];

                console.log(pid);
                if (pid > 0) {
                    console.log("kill -9 " + pid);
                    exec("kill -9 " + pid);
                }
            });

            console.log("before 1st sleep");
            sleep(1000, function() {
                console.log("after 1st sleep, start picture");
                exec('fswebcam -i 0 -d v4l2:/dev/video0 --no-banner -p YUYV --jpeg 95 --save /Media/SD-P1/test.jpg', function (error, stdout, stderr) {
                    console.log('stdout: ' + stdout);
                    console.log('stderr: ' + stderr);
                    if (error !== null) {
                        console.log('exec error: ' + error);
                    }

                    console.log("before upload");
                    fs.readFileAsync('/Media/SD-P1/test.jpg')
                        .then(function(data) {
                            myApp.emit('image','', new Buffer(data).toString('base64'));
                        });
                    console.log("after upload");

                }); 
            });
            console.log("after picture");

            console.log("before 2nd sleep");
            sleep(3000, function() {
                console.log("after 2nd sleep, start webcam");
                exec('ffmpeg -s ' + width + 'x' + height + ' -f video4linux2 -r 30 -i /dev/video0 -f mpeg1video -r 30 -b 800k http://stream-mcs.mediatek.com/' + deviceId + '/' +deviceKey + '/' + dataChnId + '/' + width + '/' + height, function(error, stdout, stderr) {

                    console.log('stdout: ' + stdout);
                    console.log('stderr: ' + stderr);
                    if (error !== null) {
                        console.log('exec error: ' + error);    
                    }
                }
                );
            });
            console.log("after webcam");
        } else {                                   
            console.log('off');                      
            fs.readFileAsync('/Media/SD-P1/pika.png')           
                .then(function(data) {
                    myApp.emit('image','', new Buffer(data).toString('base64'));
                });   

        }                                          
    });                

});

