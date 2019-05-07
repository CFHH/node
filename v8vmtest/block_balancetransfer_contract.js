function Process(param) {
    var obj = JSON.parse(param.param2);
    bc.BalanceTransfer(obj.from, obj.to, obj.amount)
}

exports.Process = Process;
