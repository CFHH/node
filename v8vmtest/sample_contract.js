var util_nouse = require('testlib/util/util.js');
var util = require('testlib/util/util.js');
var Long = require('testlib/long.js').Long

function Process(param)
{
    if (!output[param.param2])
    {
        output[param.param2] = param.param1;
    }
    else
    {
        output[param.param2] = util.add(Number(output[param.param2]), param.param1);
    }
    log("Process(), " + param.param2 + " = " + output[param.param2]);

    var l1 = Long.fromString("1234567897531");
    var l2 = Long.fromString("1234567898642");
    var l3 = l1.add(l2)
    log(l1.toString() + " + " + l2.toString() + " = " + l3.toString());

    var b1 = BigInt(1234567897531);
    var b2 = BigInt(1234567898642);
    var b3 = b1 + b2;
    log(b1.toString() + " + " + b2.toString() + " = " + b3.toString());

    var obj = {};
    obj.name = typeof b1;
    obj.vint = 3;
    //obj.vbigint = b3;
    obj.arr = new Array();
    var str = JSON.stringify(obj);
    log("obj = " + str);
    obj.arr[0] = "zzw";
    str = JSON.stringify(obj);
    log("obj = " + str);
    return 0;
}

exports.Process = Process;

