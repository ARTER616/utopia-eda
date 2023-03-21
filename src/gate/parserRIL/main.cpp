#include <iostream>
#include <fstream>
#include "passes/techmap/libparse.h"
#include <kernel/yosys.h>
#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "base/model/signal.h"
#include "gate/simulator/simulator.h"
#include <map>
#include <string>
#include <algorithm>

using namespace eda::gate::model;

int root;
void print_modules(const int ind, std::ostream &out, std::ofstream &fout){
    for (const auto &it1 : Yosys::RTLIL::IdString::global_id_index_){
        if (it1.second == ind){
            out << it1.first << " - module of index: " << it1.second << "\n";
        }
    }
}

std::map<int, std::string> INPUTS;
std::map<int, std::string> OUTPUTS;
std::map<int, std::string > typpfunc;
std::vector<int> WIRES;

void print_wires(const Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::RTLIL::Wire*> &wires, std::ostream &out, std::ofstream &fout, eda::gate::model::GNet &net,std::map<unsigned, Gate::Id> &inputs,std::map<unsigned, Gate::Id> &outputs){
    out << "Wires:" << "\n";
    for (auto it1=wires.begin(); it1 != wires.end(); ++it1){
        std::string temp;
        std::string width = "u:";
        unsigned index;
        for(const auto &it2 : Yosys::RTLIL::IdString::global_id_index_){
            if (it2.second == it1->first.index_){
                out << "  " << it2.first << " - wire of index: " << it2.second << "\n";
                index=it2.second;
                temp=it2.first;
            }
        }
        width.append(std::to_string(it1->second->width));
        if (it1->second->width>1){
            out<< "    type: bus with width " << it1->second->width << "\n";
        }
        else{
            out << "    type: wire" << "\n";
        }
        out << "    start_offset: " << it1->second->start_offset << "\n";
        out << "    port_id: " << it1->second->port_id << "\n";
        out << "    port_input: " << it1->second->port_input << "\n";
        if (it1->second->port_input==1){
            Gate::Id inputId = net.addIn();
            inputs.emplace(index, inputId);
            temp.erase(0, 1);
            fout << "input " << width << " " << temp << ";\n";
            INPUTS.emplace(index, temp);
        }
        out << "    port_output: " << it1->second->port_output << "\n";
        if (it1->second->port_output==1){
            Gate::Id outputId;
            outputs.emplace(index, outputId);
            temp.erase(0, 1);
            fout << "output " << width << " " << temp << ";\n";
            OUTPUTS.emplace(index, temp);
        }
        if (it1->second->port_output==0 && it1->second->port_input==0){
            WIRES.push_back(index);
        }
        out << "    upto: " << it1->second->upto << "\n";
        out << "\n";
    }
}
std::string function(size_t type){
    std::string func;

    if (type==ID($and).index_){
        func = "&";
    }
    if (type==ID($not).index_){
        func = "~";
    }
    if (type==ID($or).index_){
        func = "|";
    }
    return func;
}

std::map<int, std::pair<int, int>> CELL;
std::string buildRIL(int root){
        bool flag1=0,flag2=0;
        if(INPUTS.find(CELL.find(root)->second.first)!=INPUTS.end()){
            flag1=1;
        }
        if(INPUTS.find(CELL.find(root)->second.second)!=INPUTS.end()){
            flag2=1;
        }
        if (flag1==1&&flag2==1){
            if (CELL.find(root)->second.first!=CELL.find(root)->second.second){
                return INPUTS.find(CELL.find(root)->second.first)->second + typpfunc.find(CELL.find(root)->first)->second + INPUTS.find(CELL.find(root)->second.second)->second;
            }
            else{
                return typpfunc.find(CELL.find(root)->first)->second + INPUTS.find(CELL.find(root)->second.second)->second;
            }
        }
        if (flag1==0&&flag2==0){
            if (CELL.find(root)->second.first!=CELL.find(root)->second.second){
                return "(" + buildRIL(CELL.find(root)->second.first) + typpfunc.find(CELL.find(root)->first)->second + buildRIL(CELL.find(root)->second.second) + ")";
            }
            else{
                return "(" + typpfunc.find(CELL.find(root)->first)->second + buildRIL(CELL.find(root)->second.first) + ")";
            }

        }
        if (flag1==0&&flag2==1){
            return "(" + buildRIL(CELL.find(root)->second.first) + typpfunc.find(CELL.find(root)->first)->second + INPUTS.find(CELL.find(root)->second.second)->second + ")";
        }
        if (flag1==1&&flag2==0){
            return "(" + INPUTS.find(CELL.find(root)->second.first)->second + typpfunc.find(CELL.find(root)->first)->second + buildRIL(CELL.find(root)->second.second) + ")";
        }
}

//Gate::Id buildNet(int root,eda::gate::model::GNet &net,std::map<int,GateSymbol> typeFunc,std::map<int, std::pair<int, int>> &cell,std::map<unsigned, Gate::Id> &inputs){
//    for (auto it: cell){
//        if (it.second.first==root){
//            if (typeFunc.find(it.first)->second==GateSymbol::LATCH){
//                return net.addLatch(inputs.find(it.first)->second,inputs.find(it.second.second)->second);
//            }
//            if (typeFunc.find(it.first)->second==GateSymbol::DFF){
//                return net.addDff(inputs.find(it.first)->second,inputs.find(it.second.second)->second);
//            }
//            if(typeFunc.find(it.first)->second==GateSymbol::DFFrs){
//                int key;
//                for(auto it1: cell){
//                    if (it1.second.second==it.first){
//                        key=it1.first;
//                    }
//                }
//                return net.addDffrs(inputs.find(it.first)->second,inputs.find(cell.find(it.first)->second.second)->second,inputs.find(cell.find(key)->second.first)->second,inputs.find(cell.find(key)->first)->second );
//            }
//        }
//    }
//    bool flag1=0,flag2=0;
//    if(inputs.find(cell.find(root)->second.first)!=inputs.end()){
//        flag1=1;
//    }
//    if(inputs.find(cell.find(root)->second.second)!=inputs.end()){
//        flag2=1;
//    }
//    if (flag1==1&&flag2==1){
//        Gate::SignalList inputs1;
//        if (cell.find(root)->second.first!=cell.find(root)->second.second){
//            inputs1.push_back(Gate::Signal::always(inputs.find(cell.find(root)->second.first)->second));
//            inputs1.push_back(Gate::Signal::always(inputs.find(cell.find(root)->second.second)->second));
//        }
//        else{
//            inputs1.push_back(Gate::Signal::always(inputs.find(cell.find(root)->second.first)->second));
//        }
//        return net.addGate(typeFunc.find(root)->second,inputs1);
//    }
//    if (flag1==0&&flag2==0){
//        Gate::SignalList inputs1;
//        if (cell.find(root)->second.first!=cell.find(root)->second.second){
//            inputs1.push_back(Gate::Signal::always(buildNet(cell.find(root)->second.first,net,typeFunc,cell,inputs)));
//            inputs1.push_back(Gate::Signal::always(buildNet(cell.find(root)->second.second,net,typeFunc,cell,inputs)));
//        }
//        else{
//            inputs1.push_back(Gate::Signal::always(buildNet(cell.find(root)->second.first,net,typeFunc,cell,inputs)));
//        }
//        return net.addGate(typeFunc.find(root)->second,inputs1);
//    }
//    if (flag1==0&&flag2==1){
//        Gate::SignalList inputs1;
//        inputs1.push_back(Gate::Signal::always(buildNet(cell.find(root)->second.first,net,typeFunc,cell,inputs)));
//        inputs1.push_back(Gate::Signal::always(inputs.find(cell.find(root)->second.second)->second));
//        return net.addGate(typeFunc.find(root)->second,inputs1);
//    }
//    if (flag1==1&&flag2==0){
//        Gate::SignalList inputs1;
//        inputs1.push_back(Gate::Signal::always(inputs.find(cell.find(root)->second.first)->second));
//        inputs1.push_back(Gate::Signal::always(buildNet(cell.find(root)->second.second,net,typeFunc,cell,inputs)));
//        return net.addGate(typeFunc.find(root)->second,inputs1);
//    }
//}

void print_cells(const Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::RTLIL::Cell*> &cells, std::ostream &out, std::ofstream &fout, eda::gate::model::GNet &net, std::map<int,GateSymbol> &typeFunc,std::map<int, std::pair<int, int>> &cell){
    out << "Cells:" << "\n";
    for (auto it1=cells.begin(); it1 != cells.end(); ++it1){
        for (const auto &it2 : Yosys::RTLIL::IdString::global_id_index_){
            if (it2.second == it1->first.index_){
                out << "  " << it2.first << " cell of index " << it2.second << " type " << it1->second->type.index_ << "\n";
            }
        }
        size_t i=0;
        bool flag=0;
        //GateSymbol f;
        std::string symbol;
        int a,b,c;
        for (auto it3 = it1->second->connections_.begin(); it3 != it1->second->connections_.end(); ++it3){
            i++;
            if (i==1){
                symbol = function(it1->second->type.index_);
                a=it3->second.as_wire()->name.index_;
                symbol=function(it1->second->type.index_);
                typpfunc.emplace(a,symbol);
                if (symbol=="~"){
                    flag=1;
                }
            }
            if (i==2){
                if (!flag){
                    b=it3->second.as_wire()->name.index_;
                    flag=0;
                }
                else{
                    b=it3->second.as_wire()->name.index_;
                    c=b;
                    CELL.emplace(a,std::make_pair(b,c));
                }
            }
            if (i==3){
                c=it3->second.as_wire()->name.index_;
                CELL.emplace(a,std::make_pair(b,c));
            }
            if (i==4){
                a=it3->second.as_wire()->name.index_;
            }
            if (i==5){
                b=it3->second.as_wire()->name.index_;
                CELL.emplace(c,std::make_pair(a,b));
                typpfunc.emplace(c,symbol);
            }
            out << "    Connections: " << it3->first.index_ << "\n";
            out << "      name: " << it3->second.as_wire()->name.index_ << "\n";
        }
    }
}
void print_connections(const std::vector<std::pair<Yosys::RTLIL::SigSpec, Yosys::RTLIL::SigSpec> > &connections, std::ostream &out, std::ofstream &fout, eda::gate::model::GNet &net,std::map<int,GateSymbol> typeFunc,std::map<int, std::pair<int, int>> &cell,std::map<unsigned, Gate::Id> &inputs,std::map<unsigned, Gate::Id> &outputs){
    out << "Connections count: " << connections.size() << "\n";
    for (auto it1 = connections.begin(); it1 != connections.end(); ++it1){
        out << "    Wire " << it1->first.as_wire()->name.index_ << " connects with " << it1->second.as_wire()->name.index_ << "\n";
        if(inputs.find(it1->second.as_wire()->name.index_)==inputs.end()){
            root=it1->second.as_wire()->name.index_;
            fout << "@(*) {\n";
            fout << "   " << OUTPUTS.find(it1->first.as_wire()->name.index_)->second << " = " << buildRIL(root) << ";\n";
            fout << "}\n";
        }
        else{
            fout << "@(*) {\n";
            fout << "   " << OUTPUTS.find(it1->first.as_wire()->name.index_)->second << " = " << INPUTS.find(it1->second.as_wire()->name.index_)->second << ";\n";
            fout << "}\n";
//            Gate::SignalList inputs1;
//            inputs1.push_back(Gate::Signal::always(inputs.find(it1->second.as_wire()->name.index_)->second));
//            auto output=net.addGate(GateSymbol::NOP,inputs1);
//            outputs.find(it1->first.as_wire()->name.index_)->second=net.addOut(output);
//            std::cout << net;
        }
    }
}

void print_memory(const Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::RTLIL::Memory*> &memories, std::ostream &out, std::ofstream &fout){
    out << "Memory count: " << memories.size() << "\n";

    for (auto it1 = memories.begin(); it1 != memories.end(); ++it1){

        out << "  memory " << it1->second << "\n";
        out << "  name: " << it1->second->name.index_ << "\n";
        out << "  width: " << it1->second->width << "\n";
        out << "  start_offset: " << it1->second->start_offset << "\n";
        out << "  size: " << it1->second->size << "\n";
        out << "\n";
    }
}

std::vector<int> seq;
std::vector<int> clk;

void print_actions(const std::vector< Yosys::RTLIL::SigSig > &actions, std::ostream &out, std::ofstream &fout){
    int z=0;
    for (auto &it : actions){
        z++;
        out << z <<"\n";
        out << "    ch" << (it.first.is_wire())<< " "<<(it.second.is_wire())<< "\n";
        if (it.first.is_wire() && it.second.is_wire()){
            out << "    nech" << (it.first.as_wire()->name.index_)<< " "<<(it.second.as_wire()->name.index_)<< "\n";
            if (OUTPUTS.find((it.first.as_wire()->name.index_)) != OUTPUTS.end()){
                seq.insert(seq.begin(), (it.first.as_wire()->name.index_));
            }
            else{
                seq.push_back((it.first.as_wire()->name.index_));
                seq.push_back((it.second.as_wire()->name.index_));
            }

        }
        out << "    ch" << (it.first.is_chunk())<< " "<<(it.second.is_chunk())<< "\n";
        if (it.first.is_chunk() && it.second.is_chunk()){
            out << "    nech" << (it.first.as_chunk().wire)->name.index_<< " "<<(it.second.as_chunk().wire)->name.index_<< "\n";
        }
        out << "    ch" << (it.first.is_fully_const()) <<" "<<(it.second.is_fully_const()) << "\n";
//        if (it.first.is_fully_const() && it.second.is_fully_const()){
//            out << "    nech" << (it.first.as_fully_const())<< " "<<(it.second.as_fully_const())<< "\n";
//        }
        out << "    ch" << (it.first.is_fully_def()) <<" "<<(it.second.is_fully_def()) << "\n";
//        if (it.first.is_fully_def() && it.second.is_fully_def()){
//            out << "    nech" << (it.first.as_fully_def())<< " "<<(it.second.as_fully_def())<< "\n";
//        }
        out << "    ch" << (it.first.is_fully_undef()) <<" "<<(it.second.is_fully_undef()) << "\n";
//        if (it.first.is_fully_undef() && it.second.is_fully_undef()){
//            out << "    nech" << (it.first.as_fully_undef())<< " "<<(it.second.as_fully_undef())<< "\n";
//        }

    }
}
void print_syncs(const std::vector<Yosys::RTLIL::SyncRule *> &syncs, std::ostream &out, std::ofstream &fout){
    out << "  syncs\n";
    for (auto it1=syncs.begin(); it1 != syncs.end(); ++it1){
        out << "    type " << (*it1)->type << "\n";
        out << "  signal\n";
        out << "    size " << (*it1)->signal.size() << "\n";
        out << "    as_wire index " << (*it1)->signal.as_wire()->name.index_ << "\n";
        clk.push_back((*it1)->signal.as_wire()->name.index_);
        out << "  actions\n";
        print_actions((*it1)->actions, out, fout);
    }
    out <<"zzzzzzzzzzss" <<INPUTS.size();
    fout << "\n@(*) if (" << INPUTS.find(clk[0])->second << ") {\n    " << OUTPUTS.find(seq[0])->second << " = " << INPUTS.find(seq[2])->second << ";\n}\n";
}

std::vector<int> LEFT, RIGHT;

void print_processes(const Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::RTLIL::Process*> &processes, std::ostream &out, std::ofstream &fout){
    out << "Processes count: " << processes.size() << "\n";
    for (auto it1 = processes.begin(); it1 != processes.end(); ++it1){
        out << "  name " << it1->second->name.index_ << "\n";
        out << "  root_case: \n";
        print_actions((it1->second->root_case).actions, out, fout);
        for (auto &it2 : (it1->second->root_case).switches){
            out << "    " << (it2->signal.as_chunk().wire)->name.index_ << "\n";
        }
        print_syncs(it1->second->syncs, out, fout);
        out << "\n";
    }
}

void print_ports(const std::vector<Yosys::RTLIL::IdString> &ports, std::ostream &out, std::ofstream &fout){
    out << "Ports count: " << ports.size() << "\n";
    for (auto &it1 : ports){
        out << "  port " << it1.index_ << "\n";
    }
}

void print_params(const std::pair<Yosys::RTLIL::IdString, Yosys::RTLIL::Module*> &m, std::ostream &out, std::ofstream &fout){
    eda::gate::model::GNet net(0);
    std::map<unsigned, Gate::Id> outputs;
    std::map<unsigned, Gate::Id> inputs;
    std::map<int, std::pair<int, int>> cell;
    std::map<int,GateSymbol> typeFunc;
    print_modules(m.first.index_, out, fout);
    out << "\n";
    print_wires(m.second->wires_, out, fout, net, inputs, outputs);
    out << "\n";
    print_cells(m.second->cells_, out, fout, net, typeFunc, cell);
    out << "\n" ;
    print_connections(m.second->connections_, out, fout, net, typeFunc, cell, inputs, outputs);
    out << "\n";
    print_memory(m.second->memories, out, fout);
    out << "\n";
    print_processes(m.second->processes, out, fout);
    out << "\n";
    print_ports(m.second->ports, out, fout);
    out << "\n";

    out << "Refcount wires count: " << m.second->refcount_wires_ << "\n";
    out << "Refcount cells count: " << m.second->refcount_cells_ << "\n";
    out << "Monitors count: " << m.second->monitors.size() << "\n";
    out << "Avail_parameters count: " << m.second->avail_parameters.size() << "\n";
    out << "\n";
}

void print_parsed(const Yosys::RTLIL::Design &des, std::ostream &out, std::ofstream &fout){
    for (auto &m: des.modules_){
        print_params(m, out, fout);
    }
}
int main(int argc, char* argv[]){
    std::ostream& out = std::cout;
    std::ofstream fout("temp.txt");
    Yosys::yosys_setup();
    for (size_t o=1;o<argc;++o){
        Yosys::RTLIL::Design design;
        Yosys::run_frontend(argv[o], "verilog", &design, nullptr);
        print_parsed(design, out, fout);
    }
    fout.close();
    Yosys::yosys_shutdown();
}