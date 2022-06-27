from distutils.cmd import Command
from re import M
import sys
import json
import matplotlib.pyplot as plt


# plt.figure()

# labels = []
# while(True):
#     file_path = input("Enter path to benchmark output file: ")

#     if file_path.lower() == "exit":
#         break

#     label = input("Enter the above graph's label: ")
#     labels.append(label)

#     x, y = [], []
#     read_json_results(x, y, file_path)

#     plt.plot(x, y, label=label)

# plt.xscale('log', base=2)
# plt.yscale('log', base=2)
# plt.legend()
# plt.savefig("_vs_".join(labels) + "_graphed")

class CommandLineArgument:
    def __init__(self, arg : str, on_process, **kwargs):
        #complying with GNU standard long and short options, respectively.
        if len(arg) > 1:
            self.__arg = "--" + arg
        else:
            self.__arg = "-" + arg

        self.__on_process = on_process

        self.__is_switch   = kwargs.get("switch", len(arg) == 1)

        if not self.__is_switch:
            self.__is_list     = kwargs.get("list", False)
        else:
            raise ValueError("Switch argument cannot simulataneously accept list!")

        self.__is_first    = kwargs.get("first", False)
        self.__is_last     = kwargs.get("last", False)
        self.__is_required = kwargs.get("required", False)

    def process(self, arg : str, position, **kwargs):
        switch_value_pair = arg.split("=")

        if(self.__is_switch and len(switch_value_pair) > 1):
            raise ValueError("Unexcepted value passed to argument:" + self.__arg + ", should be a switch!")
        else:
            value = switch_value_pair[1]

            if self.__is_list:
                self.__on_process(value.split(";"), **kwargs)
            else:
                self.__on_process(value, **kwargs)
        
        return (self.__is_first, self.__is_last)

    @property
    def arg(self):
        return self.__arg

    @property
    def required(self):
        return self.__is_required

class CommandLineParser:
    def __init__(self, *args : list[CommandLineArgument]):
        self.__args = args
        self.__prev_arg = ""
        self.__last_switch = False
        self.__position = 0
        self.__required = {}

        for arg in args:
            if arg.required:
                self.__required[arg.arg] = False


    def next(self, arg: str, **kwargs):
        if self.__last_switch:
            raise ValueError(self.__prev_arg +  " was expected to be the last argument!")

        for expected_arg in self.__args:
            if(arg.startswith(expected_arg.arg)):
                exp_first, exp_last = expected_arg.process(arg, self.__position, **kwargs)
                self.__last_switch = exp_last

                if self.__position > 0 and exp_first:
                    raise ValueError(expected_arg.arg + " was expected to be the first argument!")

                if expected_arg.arg in self.__required:
                    self.__required[expected_arg.arg] = True

                self.__prev_arg = expected_arg.arg
                self.__position += 1
                return

        raise ValueError(arg.split("=")[0] +  " is an unknown argument! see --help")

    def verify_all_required_present(self):
        for arg, found in self.__required.items():
            if not found:
                raise ValueError(arg + " is required but not passed! see --help")


if __name__ == "__main__":
    data = {
        "paths":[], 
        "labels":[], 
        "xattr":"", 
        "yattr":""}

    def data_setter(key):
        def result(arg, **kwargs):
            kwargs["data"][key] = arg

        return result

    def on_xlabel(label, **kwargs):
        plt.xlabel(label)

    def on_ylabel(label, **kwargs):
        plt.ylabel(label)
    
    def on_xscale(args, **kwargs):
        plt.xscale(args[0], base=int(args[1]))

    def on_yscale(args, **kwargs):
        plt.yscale(args[0], base=int(args[1]))

    parser = CommandLineParser(
        CommandLineArgument("paths"    , data_setter("paths")     , required=True , list=True),
        CommandLineArgument("labels"   , data_setter("labels")    , required=True , list=True),
        CommandLineArgument("xattr"    , data_setter("xattr")     , required=True),
        CommandLineArgument("yattr"    , data_setter("yattr")     , required=True),
        CommandLineArgument("xscale"   , on_xscale                , list=True),
        CommandLineArgument("yscale"   , on_yscale                , list=True),
        CommandLineArgument("xlabel"   , on_xlabel)               ,
        CommandLineArgument("ylabel"   , on_ylabel)               ,
        CommandLineArgument("file_name", data_setter("save_name")))

    plt.figure()

    for arg in sys.argv[1:]:
        parser.next(arg, data=data)

    parser.verify_all_required_present()

    if(len(data["paths"]) != len(data["labels"])):
        raise ValueError("Mismatch in number of input files and labels!")

    for path, label in zip(data["paths"], data["labels"]):
        file = open(path)

        json_data = json.load(file)
        benchmark_data = json_data["benchmarks"]

        x, y = [], []

        for benchmark in benchmark_data:
            x.append(float(benchmark[data["xattr"]]))
            y.append(float(benchmark[data["yattr"]]))

        plt.plot(x, y, label=label)

        file.close()

    if "save_name" in data:
        plt.savefig(data["save_name"])

    plt.grid()
    plt.legend()
    plt.show()


    