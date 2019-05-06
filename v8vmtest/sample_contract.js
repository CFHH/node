//var util = require('util.js');

function Process(param)
{
    if (!output[param.param2])
    {
        output[param.param2] = param.param1;
    }
    else
    {
        output[param.param2] = Number(output[param.param2]) + param.param1
    }
    log("Process(), " + param.param2 + " = " + output[param.param2]);
}

exports.Process = Process;

