var util_nouse = require('testlib/util/util.js');
var util = require('testlib/util/util.js');

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
    return 0;
}

exports.Process = Process;

