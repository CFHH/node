'use strict';

(function bootstrapInternalLoaders(process, getBinding, getLinkedBinding, getInternalBinding) {
    const {
        apply: ReflectApply,
        deleteProperty: ReflectDeleteProperty,
        get: ReflectGet,
        getOwnPropertyDescriptor: ReflectGetOwnPropertyDescriptor,
        has: ReflectHas,
        set: ReflectSet,
    } = Reflect;

    const {
        prototype: {
            hasOwnProperty: ObjectHasOwnProperty,
        },
        create: ObjectCreate,
        defineProperty: ObjectDefineProperty,
        keys: ObjectKeys,
    } = Object;

    // Set up process.moduleLoadList
    const moduleLoadList = [];
    ObjectDefineProperty(process, 'moduleLoadList', {
        value: moduleLoadList,
        configurable: true,
        enumerable: true,
        writable: false
    });

    // Set up process.binding() and process._linkedBinding()
    {
        const bindingObj = ObjectCreate(null);

        process.binding = function binding(module) {
            module = String(module);
            let mod = bindingObj[module];
            if (typeof mod !== 'object') {
                mod = bindingObj[module] = getBinding(module);
                moduleLoadList.push(`Binding ${module}`);
            }
            return mod;
        };

        process._linkedBinding = function _linkedBinding(module) {
            module = String(module);
            let mod = bindingObj[module];
            if (typeof mod !== 'object')
                mod = bindingObj[module] = getLinkedBinding(module);
            return mod;
        };
    }

    // Set up internalBinding() in the closure
    let internalBinding;
    {
        const bindingObj = ObjectCreate(null);
        internalBinding = function internalBinding(module) {
            let mod = bindingObj[module];
            if (typeof mod !== 'object') {
                mod = bindingObj[module] = getInternalBinding(module);
                moduleLoadList.push(`Internal Binding ${module}`);
            }
            return mod;
        };
    }

    const { ContextifyScript } = process.binding('contextify');

    // Set up NativeModule
    function NativeModule(id) {
        this.filename = `${id}.js`;
        this.id = id;
        this.exports = {};
        this.reflect = undefined;
        this.exportKeys = undefined;
        this.loaded = false;
        this.loading = false;
    }

    NativeModule._source = getBinding('natives');
    NativeModule._cache = {};

    const config = getBinding('config');

    const codeCache = getInternalBinding('code_cache');
    const compiledWithoutCache = NativeModule.compiledWithoutCache = [];
    const compiledWithCache = NativeModule.compiledWithCache = [];

    // Think of this as module.exports in this file even though it is not written in CommonJS style.
    const loaderExports = { internalBinding, NativeModule };
    const loaderId = 'internal/bootstrap/loaders';

    NativeModule.require = function (id) {
        if (id === loaderId) {
            return loaderExports;
        }

        const cached = NativeModule.getCached(id);
        if (cached && (cached.loaded || cached.loading)) {
            return cached.exports;
        }

        if (!NativeModule.exists(id)) {
            const err = new Error(`No such built-in module: ${id}`);
            err.code = 'ERR_UNKNOWN_BUILTIN_MODULE';
            err.name = 'Error [ERR_UNKNOWN_BUILTIN_MODULE]';
            throw err;
        }

        moduleLoadList.push(`NativeModule ${id}`);

        const nativeModule = new NativeModule(id);

        nativeModule.cache();
        nativeModule.compile();

        return nativeModule.exports;
    };

    NativeModule.getCached = function (id) {
        return NativeModule._cache[id];
    };

    NativeModule.exists = function (id) {
        return NativeModule._source.hasOwnProperty(id);
    };

    if (config.exposeInternals) {
        NativeModule.nonInternalExists = function (id) {
            // Do not expose this to user land even with --expose-internals
            if (id === loaderId) {
                return false;
            }
            return NativeModule.exists(id);
        };

        NativeModule.isInternal = function (id) {
            // Do not expose this to user land even with --expose-internals
            return id === loaderId;
        };
    } else {
        NativeModule.nonInternalExists = function (id) {
            return NativeModule.exists(id) && !NativeModule.isInternal(id);
        };

        NativeModule.isInternal = function (id) {
            return id.startsWith('internal/') || (id === 'worker_threads' && !config.experimentalWorker);
        };
    }

    NativeModule.getSource = function (id) {
        return NativeModule._source[id];
    };

    NativeModule.wrap = function (script) {
        return NativeModule.wrapper[0] + script + NativeModule.wrapper[1];
    };

    NativeModule.wrapper = [
        '(function (exports, require, module, process) {',
        '\n});'
    ];

    const getOwn = (target, property, receiver) => {
        //Reflect.apply(targetFunction, thisArgument, argumentsList) = thisArgument.targetFunction(argumentsList)
        return ReflectApply(ObjectHasOwnProperty, target, [property]) ?
            ReflectGet(target, property, receiver) :
            undefined;
    };

    NativeModule.prototype.compile = function () {
        let source = NativeModule.getSource(this.id);
        source = NativeModule.wrap(source);

        this.loading = true;

        try {
            // Arguments: code, filename, lineOffset, columnOffset, cachedData, produceCachedData, parsingContext
            const script = new ContextifyScript( source, this.filename, 0, 0, codeCache[this.id], false, undefined);

            if (!codeCache[this.id] || script.cachedDataRejected) {
                compiledWithoutCache.push(this.id);
            } else {
                compiledWithCache.push(this.id);
            }

            // Arguments: timeout, displayErrors, breakOnSigint
            const fn = script.runInThisContext(-1, true, false);
            const requireFn = NativeModule.require;
            // call NativeModule.wrapper function
            fn(this.exports, requireFn, this, process);

            if (config.experimentalModules && !NativeModule.isInternal(this.id)) {
                this.exportKeys = ObjectKeys(this.exports);

                const update = (property, value) => {
                    if (this.reflect !== undefined && ReflectApply(ObjectHasOwnProperty, this.reflect.exports, [property]))
                        this.reflect.exports[property].set(value);
                };

                const handler = {
                    __proto__: null,
                    defineProperty: (target, prop, descriptor) => {
                        // Use `Object.defineProperty` instead of `Reflect.defineProperty` to throw the appropriate error if something goes wrong.
                        ObjectDefineProperty(target, prop, descriptor);
                        if (typeof descriptor.get === 'function' && !ReflectHas(handler, 'get')) {
                            handler.get = (target, prop, receiver) => {
                                const value = ReflectGet(target, prop, receiver);
                                if (ReflectApply(ObjectHasOwnProperty, target, [prop]))
                                    update(prop, value);
                                return value;
                            };
                        }
                        update(prop, getOwn(target, prop));
                        return true;
                    },
                    deleteProperty: (target, prop) => {
                        if (ReflectDeleteProperty(target, prop)) {
                            update(prop, undefined);
                            return true;
                        }
                        return false;
                    },
                    set: (target, prop, value, receiver) => {
                        const descriptor = ReflectGetOwnPropertyDescriptor(target, prop);
                        if (ReflectSet(target, prop, value, receiver)) {
                            if (descriptor && typeof descriptor.set === 'function') {
                                for (const key of this.exportKeys) {
                                    update(key, getOwn(target, key, receiver));
                                }
                            } else {
                                update(prop, getOwn(target, prop, receiver));
                            }
                            return true;
                        }
                        return false;
                    }
                };

                this.exports = new Proxy(this.exports, handler);
            }

            this.loaded = true;
        } finally {
            this.loading = false;
        }
    };

    NativeModule.prototype.cache = function () {
        NativeModule._cache[this.id] = this;
    };

    return loaderExports;
});
