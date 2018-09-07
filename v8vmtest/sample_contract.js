function Initialize()
{
}

function Process(param)
{
    if (!output[param.param2])
    {
        output[param.param2] = 1;
    } else
    {
        output[param.param2]++
    }
    log("param.param2 = " + output[param.param2]);
}

Initialize();
