'use strict';

/*
 * 简单和安全起见，所有的智能合约文件都必须保存在SetJSSourcePath指定的目录里，文件查找以此为根目录
 */

const CHAR_DOT = 46;            //'.'
const CHAR_FORWARD_SLASH = 47;  //'/'
const ROOT_PATH = "/";

function updateChildren(parent, child, scan) {
    var children = parent && parent.children;
    if (children && !(scan && children.includes(child)))
        children.push(child);
}

function makeRequireFunction(mod) {
    function require(path) {
        try {
            return mod.require(path);
        } finally {
        }
    }
    return require;
}

function Module(id, parent) {
    this.id = id;           //完整的相对于SourcePath的路径文件名,以/开始
    this.exports = {};      //exports、module.exports
    this.parent = parent;   //通过require被引入的Module，需要设置parent
    this.children = [];     //通过require引入了Module
    this.loaded = false;    //避免重复，是不是再加个loading
    updateChildren(parent, this, false);
}

Module._cache = Object.create(null);

Module.wrapper = [
    '(function (exports, require, module, __filename, __dirname) { ',
    '\n});'
];

Module.wrap = function (script) {
    return Module.wrapper[0] + script + Module.wrapper[1];
};

//确定文件file的完整路径文件名
Module._resolveFilename = function (file, parent, isMain) {
    var paths = Module._resolveLookupPaths(file, parent, true);
    var filename = Module._findPath(file, paths);
    if (!filename) {
        var err = new Error(`Cannot find module '${file}'`);
        err.code = 'MODULE_NOT_FOUND';
        throw err;
    }
    return filename;
};

//先确定file可能的搜索路径
Module._resolveLookupPaths = function (file, parent, newReturn) {
    var paths = [];
    if (file.charCodeAt(0) == CHAR_FORWARD_SLASH) {
        //以/开头当作绝对路径
        paths.push(ROOT_PATH);
    } else if (file.charCodeAt(0) == CHAR_DOT) {
        //以点开头，只能当作相对路径
        if (parent)
            paths.push(parent.getpath());
    } else {
        var parent_path = null;
        if (parent) {
            parent_path = parent.getpath();
            paths.push(parent_path);
        }
        if (parent_path != ROOT_PATH)
            paths.push(ROOT_PATH);
    }
    return paths;
};

//再在paths中确定file的完整路径文件名
Module._findPath = function (file, paths) {
    for (var index = 0; index < paths.length; index++) {
        const curPath = paths[index];
        if (file.charCodeAt(0) == CHAR_FORWARD_SLASH) {
            var filename = file;
            if (fs.IsFileExists(filename))
                return filename;
        } else if (file.charCodeAt(0) == CHAR_DOT) {
            var start = 0;
            var n = 0;
            while (true) {
                var find_slash = false;
                for (var i = start + 1; i < file.length; ++i) {
                    if (file.charCodeAt(i) == CHAR_FORWARD_SLASH) {
                        find_slash = true;
                        if (i - start == 1) {
                            start = i + 1;
                        } else if (i - start == 2) {
                            start = i + 1;
                            ++n;
                        } else {
                            return;
                        }
                        break;
                    }
                }
                if (!find_slash || start >= file.length)
                    return;
                if (file.charCodeAt(start) != CHAR_DOT)
                    break;
            }
            if (n > 0) {
                var end = 0;
                for (var i = curPath.length - 1 - 1; i >= 0; --i) {  //curPath以'/'结尾
                    if (curPath.charCodeAt(i) == CHAR_FORWARD_SLASH) {
                        --n;
                        if (n == 0) {
                            end = i;
                            break;
                        }
                    }
                }
                if (n != 0)
                    return;
                var p1 = curPath.slice(0, end + 1);
                var p2 = file.slice(start);
                var filename = curPath.slice(0, end + 1).concat(file.slice(start));
                if (fs.IsFileExists(filename))
                    return filename;
            } else {
                var filename = curPath.cancat(file.slice(start));
                if (fs.IsFileExists(filename))
                    return filename;
            }
        } else {
            var filename = curPath.concat(file);
            if (fs.IsFileExists(filename))
                return filename;
        }
    }
};

Module._load = function (file, parent, isMain) {
    var parent_id = "(NO PARENT)";
    if (parent)
        parent_id = parent.id;
    log("LOAD MODULE : " + file + ", parnet = " + parent_id);
    var filename = Module._resolveFilename(file, parent, isMain);
    log("    FULL PATH : " + filename);
    var cachedModule = Module._cache[filename];
    if (cachedModule) {
        log("    CACHED MODULE: " + filename);
        updateChildren(parent, cachedModule, true);
        return cachedModule.exports;
    }
    var module = new Module(filename, parent);
    Module._cache[filename] = module;
    Module._tryLoad(module, filename);
    return module.exports;
};

Module._tryLoad = function (module, filename) {
    var threw = true;
    try {
        module.load(filename);
        threw = false;
    } finally {
        if (threw) {
            delete Module._cache[filename];
        }
    }
};

Module.prototype.getpath = function () {
    var filename = this.id;
    for (var i = filename.length - 1; i >= 0; --i) {
        if (filename.charCodeAt(i) == CHAR_FORWARD_SLASH) {
            return filename.slice(0, i + 1);
        }
    }
    return ROOT_PATH;
};

Module.prototype.load = function (filename) {
    if (this.loaded)
        return;
    var module_fn = sys.LoadScript(filename);
    var require_fn = makeRequireFunction(this);
    module_fn(this.exports, require_fn, this, this.filename, null);
    this.loaded = true;
};

//模块内部的require
Module.prototype.require = function (file) {
    return Module._load(file, this, false);
};

//暴露给C++，载入一个智能合约的入口
Module.runMain = function (file) {
    return Module._load(file, null, true);
};

function runMain(file) {
    return Module._load(file, null, true);
}
