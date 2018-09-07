function Initialize()
{
}

function Process(param)
{
    if (!output[param.param2])
    {
        output[param.param2] = param.param1;
    } else
    {
        output[param.param2] = Number(output[param.param2]) + param.param1
    }
    //output[param.param2] = param.param1
    log("Process(), " + param.param2 + " = " + output[param.param2]);
}

Initialize();
