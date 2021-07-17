/************************************************************************
 * 
 * Prathmesh CS19B037
 * Hemank    CS19B058
 * Divya     CS19B067
 * 
 * Change :
 * Branch Instruction is resolved in ID stage 
 * 
 * Which affects the stalls calculation and overall CPI value 
 * **********************************************************************/
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <bitset>

using namespace std;

/*
Operation of Instruction Fetch - Control Unit generates the control signals that copy an instruction
byte from the memory into the Instruction Register, IR. The address of this
instruction is in the Program Counter, PC.
*/
class InstrcutionFetch
{
    public:
    bitset<16> PC;   // Program Counter
    bool run;        // flag for execution
};


/*
Operation of Instruction Decode - The bits of the instruction are decoded into control signals.
 Operands are moved from registers or immediate fields to working registers. For  branch
 instructions,the branch condition is tested and the branch address computed.
*/
class InstructionDecode
{
    public:
    bitset<16> instruction; // instruction
    bool run;               // flag for execution
};


/*
Operation of Execute - The instruction is executed. Specifically, if the instruction is
an arithmetic or logical operation, its results are computed.  If it is a load-store instruction,
the address is computed. All this is done by an elaborate logic circuit called the
arithmetic-logical unit (ALU).
*/

class Execute
{
    public:
    bitset<8> data1;        // first operand
    bitset<8> data2;        // second operand
    bitset<4> r_first;      // first operand register add.
    bitset<4> r_second;     // second operand register add.
    bitset<4> r_write_add;  // write register add.
    bool isJump;            // is jump flag
    bool read_mem;          // flag for reading from Cache   
    bool write_mem;         // flag for writing in Cache
    bool alu_op;            // flag for alu_op
    bool write_enable;      // flag to update the register
    bool run;               // flag for execution
};


/*
Operation of Memory Access- Access data cache, if needed. If the instruction is a load, data returns from the data
cache and is placed in the LMD (load memory data) register. Basically, if the instruction is a load-store,
the memory is read or written.
*/
class MemoryAccess
{
    public:
    bitset<8> aluResult;    // result of ALU
    bitset<8> storeData;    // data to be stored 
    bitset<4> r_first;      // first operand register address
    bitset<4> r_second;     // second operand register address
    bitset<4> r_write_add;  // write register address
    bool read_mem;          // flag for reading from Cache
    bool write_mem;         // flag for wrtiing in cache
    bool write_enable;      // flag to update register
    bool run;               // flag to execute stage
};



/*
Operation of Write Back- The results of the operation are written to the destination register.
*/
class WriteBack
{
    public:
    bitset<8> writeData;    // data to be written
    bitset<4> r_first;      // first register add.
    bitset<4> r_second;     // second reg. add.
    bitset<4> r_write_add;  // write register add.
    bool write_enable;      // flag to update register
    bool run;               // flag to execute
};


/*
Defining a five-stage instruction pipeline: IF-ID-EX-MEM-WB
*/
class State
{
    // each stage of pipeline
    public:
    InstrcutionFetch IF;
    InstructionDecode ID;
    Execute EX;
    MemoryAccess MEM;
    WriteBack WB;
};

//Defining Register file class consisting of arrays of registers
class RF
{
    private:
        vector<bitset<8>> Register; // registers 

    public:
        RF()
        {
            Register.resize(16);
            // initialise R0  = 0
            Register[0] = bitset<8> (0); 

            // read content from register file
            ifstream registerFile;
            registerFile.open("RF.txt");

            int content;
            // read all registers
            for(int i=0;i<16;i++)
            {
                // convert hex to int 
                registerFile >> hex >> content;
                Register[i] = bitset<8> (content);
            }
            registerFile.close();
        }

        // Function to read data from a register
        bitset<8> readData(bitset<4> regAdd)
        {
            // return data from the address provided
            return Register[regAdd.to_ulong()];
        }

        // Function to read data 
        // 2 read port to simultaneous read
        pair<bitset<8>, bitset<8>> readData(bitset<4> regAdd1, bitset<4> regAdd2)
        {
            pair<bitset<8>, bitset<8>> p;
            p.first = Register[regAdd1.to_ulong()];
            p.second = Register[regAdd2.to_ulong()];
            return p;
        }

        // update register value 
        void writeData(bitset<4> regAdd, bitset<8> data)
        {
            Register[regAdd.to_ulong()] = data;
        }
};

/*
InstructionCache acts as a buffer memory between external memory and the DSP core processor.
When code executes,the code words at the locations requested by the instruction set are
copied into the Instruction Cache for direct access by the core processor.If the same code
is used frequently in a set of program instructions, storage of these instructions in the
cache yields an increase in throughput because external bus accesses are eliminated.
*/
class ICache
{
    private:
        vector<bitset<8>> iCache; // iCache content

    public:
        ICache()
        {
            iCache.resize(256); // 256 B

            // reaf from ICache.txt
            ifstream cacheFile;
            cacheFile.open("ICache.txt");

            int content;
            for(int i=0;i<256;i++)
            {
                cacheFile >> hex >> content;
                iCache[i] = bitset<8> (content);
            }
            cacheFile.close();
        }

        // function to read Instruction 
        bitset<16> readInstruction(bitset<16> iAdd)
        {
            // combine 2 lines to get full instruction
            string instruction = "";
            instruction.append(iCache[iAdd.to_ulong()].to_string());
            instruction.append(iCache[iAdd.to_ulong()+1].to_string());
            return bitset<16> (instruction);
        }

        void writeInstruction(bitset<16> iAdd, bitset<16> data)
        {
            iCache[iAdd.to_ulong()] = bitset<8> (data.to_string().substr(0, 8));
            iCache[iAdd.to_ulong()+1] = bitset<8> (data.to_string().substr(8, 8));
        }
};


// DCache or data cache includes cache lines fetched from memory for loading into a register as data.
class DCache
{
    private:
        vector<bitset<8>> dCache; // Data cache 

    public:
        DCache()
        {
            dCache.resize(256); // 256 B

            // read data cache content from the file 
            ifstream cacheFile;
            cacheFile.open("DCache.txt");

            int content;
            for(int i=0;i<256;i++)
            {
                cacheFile >> hex >> content;
                dCache[i] = bitset<8> (content);
            }
            cacheFile.close();
        }

        // to read data from the cache 
        bitset<8> readData(bitset<8> iAdd)
        {
            string data = "";
            data.append(dCache[iAdd.to_ulong()].to_string());
            return bitset<8> (data);
        }

        // to update data in the cache 
        void writeData(bitset<8> iAdd, bitset<8> data)
        {
            dCache[iAdd.to_ulong()] = bitset<8> (data.to_string().substr(0, 8));
        }

        // print cache content finally in a file 
        void createResultFile()
        {
            ofstream output;
            output.open("ResultDCache.txt");
            for(int i=0;i<256; i++)
            {
                int a = dCache[i].to_ulong();
                output << hex << a/16 << a%16 << endl;
            }
            output.close();
        }
};

int main()
{
    RF reg;         // registers 
    ICache iCache;  // instruction cache 
    DCache dCache;  // data cache 
    State state;    // current state of all pipeline stage
    State newState; // updated state

    // initialise PC with 0
    state.IF.PC = bitset<16> (0);

    // initially only IF stage need to be executed  
    state.IF.run = 1;

    // set run flag for other stages as 0
    state.ID.run = 0;
    state.EX.run = 0;
    state.MEM.run = 0;
    state.WB.run = 0;

    // copy to newState
    newState = state;

    bitset<16> instruction(0);  // instruction
    bitset<4> opcode(0);        // opcode 

    // if the given register is busy of not
    vector<bool> isBusyRegister(16, false); 

    // to implement stalls 
    bool fetchPaused = false;
    bool exPaused = false;
    bool memPaused = false;
    bool wbPaused = false;

    // different type of counts 
    int cycle = 0;
    int totalInstructions = 0;
    int arithmaticInstructions = 0;
    int logicalInstructions = 0;
    int dataInstructions = 0;
    int controlInstructions = 0;
    int haltInstructions = 0;
    int totalStalls = 0;
    int dataStalls = 0;
    int controlStalls = 0;
    float cpi = 0;

    while(true)
    {
        // --------------------------------------------------------------------
        //                          WB Stage {START}
        // --------------------------------------------------------------------
        if(state.WB.run && !wbPaused)
        {
            // if write enable flag is true  
            if(state.WB.write_enable)
            {
                // update register content 
                bitset<8> temp = bitset<8> (state.WB.writeData.to_ulong()); // data
                reg.writeData(state.WB.r_write_add, temp); 

                // set it as not busy
                isBusyRegister[state.WB.r_write_add.to_ulong()] = false; 
            }
        }
        // --------------------------------------------------------------------
        //                          WB Stage {END}
        // --------------------------------------------------------------------

        // -------------------------------------------------------------------
        //                      MEM Stage {START}
        // -------------------------------------------------------------------

        wbPaused = memPaused;           // set pause flags
        newState.WB.run = state.MEM.run;// set run flags  
        if(state.MEM.run && !memPaused) 
        {
            // if flag write in memory is true
            // aluResult contains address in the cache for this case 
            if(state.MEM.write_mem)
            {
                dCache.writeData(state.MEM.aluResult, state.MEM.storeData);
            }
            // if flag read from memory is true
            if(state.MEM.read_mem)
            {
                newState.WB.writeData = dCache.readData(state.MEM.aluResult);
            }
            // else forward the result to WB stage
            else
            {
                newState.WB.writeData = state.MEM.aluResult;
            }
            
            // forward data to WB stage
            newState.WB.r_first = state.MEM.r_first;
            newState.WB.r_second = state.MEM.r_second;
            newState.WB.r_write_add = state.MEM.r_write_add;
            newState.WB.write_enable = state.MEM.write_enable;
        }
        // --------------------------------------------------------------------
        //                          MEM Stage {END}
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        //                          EX Stage {START}
        // --------------------------------------------------------------------
        memPaused = exPaused;           // set pause flag 
        newState.MEM.run = state.EX.run;// set run flags 
        if(state.EX.run && !exPaused)
        {
            int op = opcode.to_ulong(); // convert in integer

            // increment counter for each instruction
            if(op == 0)     // ADD
            {
                arithmaticInstructions++;
                 bitset<8> data1 = state.EX.data1;
                 bitset<8> data2 = state.EX.data2;
                 newState.MEM.aluResult = data1.to_ulong() + data2.to_ulong();
            }
            else if(op == 1)// SUB
            {
                arithmaticInstructions++;
                newState.MEM.aluResult = state.EX.data1.to_ulong()-state.EX.data2.to_ulong();
            }
            else if(op == 2)// MUL
            {
                arithmaticInstructions++;
                newState.MEM.aluResult = state.EX.data1.to_ulong()*state.EX.data2.to_ulong();
            }
            else if(op == 3)// INC
            {
                arithmaticInstructions++;
                newState.MEM.aluResult = state.EX.data1.to_ulong() + 1;
            }
            else if(op == 4)// AND
            {
                logicalInstructions++;
                newState.MEM.aluResult = state.EX.data1 & state.EX.data2;
            }
            else if(op == 5)// OR
            {
                logicalInstructions++;
                newState.MEM.aluResult = state.EX.data1 | state.EX.data2;
            }
            else if(op == 6)// NOT
            {
                logicalInstructions++;
                newState.MEM.aluResult = ~state.EX.data1;
            }
            else if(op == 7)// XOR
            {
                logicalInstructions++;
                newState.MEM.aluResult = state.EX.data1 ^ state.EX.data2;
            }
            else if(op == 8)// address calculation
            {
                dataInstructions++;
                newState.MEM.aluResult = state.EX.data1.to_ulong() + state.EX.data2.to_ulong();
            }
            else if(op == 9)// address calculation
            {
                dataInstructions++;
                newState.MEM.aluResult = state.EX.data1.to_ulong() + state.EX.data2.to_ulong();
                // forward data to MEM stage
                newState.MEM.storeData = bitset<8> (reg.readData(state.EX.r_write_add).to_ulong());
            }

            // forward data to MEM stage
            newState.MEM.r_first = state.EX.r_first;
            newState.MEM.r_second = state.EX.r_second;
            newState.MEM.r_write_add = state.EX.r_write_add;
            newState.MEM.read_mem = state.EX.read_mem;
            newState.MEM.write_enable = state.EX.write_enable;
            newState.MEM.write_mem = state.EX.write_mem;
        }
        // --------------------------------------------------------------------
        //                          EX Stage {END}
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        //                          ID Stage {START}
        // --------------------------------------------------------------------
        newState.EX.run = state.ID.run;
        if(state.ID.run)
        {
            // opcode from instruction
            opcode = bitset<4> (state.ID.instruction.to_string().substr(0, 4));

            if(opcode.to_ulong() == 10) // JMP instruction
            {
                controlInstructions++; // increment value
                // separate offset for jump
                bitset<8> jump = bitset<8>(state.ID.instruction.to_string().substr(4,8));

                // Update PC value according to 2's Complement form 
                if(jump[7])
                {
                    state.IF.PC = state.IF.PC.to_ulong() + ((jump.to_ulong()-256)<<1);
                }
                else state.IF.PC = state.IF.PC.to_ulong() + (jump.to_ulong()<<1);
                newState.EX.run = false;
            }

            int op = opcode.to_ulong(); 

            // if op is ALU instruction involving 2 register operands 
            if(op == 0 || op == 1 || op == 2 || op == 4 || op == 5 || op == 7)
            {
                // register in which the result will be stored 
                newState.EX.r_write_add = bitset<4> (state.ID.instruction.to_string().substr(4, 4));

                // set flags
                newState.EX.write_enable = 1;
                newState.EX.alu_op = true;

                // operand registers 
                bitset<4> Rs = bitset<4> (state.ID.instruction.to_string().substr(8,4));
                bitset<4> Rt = bitset<4> (state.ID.instruction.to_string().substr(12,4));

                // check both are free or not 
                if(!isBusyRegister[Rs.to_ulong()] && !isBusyRegister[Rt.to_ulong()])
                {
                    // write register will be busy till WB stage is executed 
                    isBusyRegister[newState.EX.r_write_add.to_ulong()] = true;

                    // for values to next EX stage
                    newState.EX.r_first = Rs;
                    newState.EX.r_second = Rt;
                    bitset<8> data1 = reg.readData(Rs);
                    bitset<8> data2 = reg.readData(Rt);
                    newState.EX.data1 = bitset<8> (data1[7]?(-256+data1.to_ulong()):data1.to_ulong());
                    newState.EX.data2 = bitset<8> (data2[7]?(-256+data2.to_ulong()):data2.to_ulong());
                    newState.EX.read_mem =  false;
                    newState.EX.write_mem = false;

                    // resume the other stages 
                    fetchPaused = false;
                    exPaused = false;
                }
                else
                {
                    // increment stall counters 
                    totalStalls++;
                    dataStalls++;

                    // pause fetch and ex stage 
                    fetchPaused = true;
                    exPaused = true;
                }
            }
            else if(op == 3) // INC
            {
                // operand register is same as write register 
                bitset <4> R = bitset<4> (state.ID.instruction.to_string().substr(4, 4));
                newState.EX.r_write_add = R.to_ulong();

                // check the register is busy or not 
                if(!isBusyRegister[R.to_ulong()])
                {
                    // forward results to next stage 
                    newState.EX.write_enable = 1;
                    newState.EX.alu_op = true;
                    isBusyRegister[R.to_ulong()] = true;
                    bitset<8> data1 = reg.readData(R);
                    newState.EX.data1 = bitset<8> (data1[7]?(-256+data1.to_ulong()):data1.to_ulong());
                    newState.EX.read_mem = false;
                    newState.EX.write_mem = false;

                    // resume stages
                    fetchPaused = false;
                    exPaused = false;
                }
                else
                {
                    // increment counters and pause stages 
                    totalStalls++;
                    dataStalls++;
                    fetchPaused = true;
                    exPaused = true;
                }
            }
            else if(op == 8) // LOAD 
            {
                // register in which value is goint to be loaded 
                newState.EX.r_write_add = bitset<4> (state.ID.instruction.to_string().substr(4, 4));

                // base address 
                bitset<4> R = bitset<4> (state.ID.instruction.to_string().substr(8, 4));
                if(!isBusyRegister[R.to_ulong()])
                {
                    // forward result to next stage and resume stages 
                    newState.EX.write_enable = 0;
                    newState.EX.alu_op = 1;
                    isBusyRegister[newState.EX.r_write_add.to_ulong()] = false;
                    bitset<8> data1 = reg.readData(R);
                    newState.EX.data1 = bitset<8> (data1.to_ulong());
                    newState.EX.data2 = bitset<8> (state.ID.instruction.to_string().substr(12, 4));
                    newState.EX.read_mem = true;
                    newState.EX.write_mem = false;
                    fetchPaused = false;
                    exPaused = false;
                }
                else
                {
                    totalStalls++;
                    dataStalls++;
                    fetchPaused = true;
                    exPaused = true;
                }
            }
            else if(op == 6)// NOT  similar to INC
            {
                newState.EX.r_write_add = bitset<4> (state.ID.instruction.to_string().substr(4, 4));
                bitset<4> R = bitset<4> (state.ID.instruction.to_string().substr(8, 4));
                if(!isBusyRegister[R.to_ulong()])
                {
                    newState.EX.write_enable = 1;
                    newState.EX.alu_op = 1;
                    isBusyRegister[newState.EX.r_write_add.to_ulong()] = true;
                    bitset<8> data1 = reg.readData(R);
                    newState.EX.data1 = bitset<8> (data1.to_ulong());
                    newState.EX.read_mem = false;
                    newState.EX.write_mem = false;
                    fetchPaused = false;
                    exPaused = false;
                }
                else
                {
                    totalStalls++;
                    dataStalls++;
                    fetchPaused = true;
                    exPaused = true;
                }
            }
            else if(op == 9) // STORE Instruction 
            {
                bitset<4> Rs = bitset<4> (state.ID.instruction.to_string().substr(4, 4));
                bitset<4> Rt = bitset<4> (state.ID.instruction.to_string().substr(8, 4));
                if(!isBusyRegister[Rs.to_ulong()] && !isBusyRegister[Rt.to_ulong()])
                {
                    newState.EX.write_mem = true;
                    newState.EX.read_mem = false;
                    newState.EX.alu_op = 1;
                    newState.EX.r_write_add = Rs;
                    newState.EX.write_enable = false;
                    bitset<8> data1 = reg.readData(Rt);
                    newState.EX.data1 = bitset<8> (data1.to_string());
                    newState.EX.data2 = bitset<8> (state.ID.instruction.to_string().substr(12, 4));
                    fetchPaused = false;
                    exPaused = false;
                }
                else
                {
                    totalStalls++;
                    dataStalls++;
                    fetchPaused = true;
                    exPaused = true;
                }
            }
            else if(op == 11) // Branch Instruction 
            {
                // register whose content is goint to be compared with 0
                bitset<4> R = bitset<4> (state.ID.instruction.to_string().substr(4, 4));

                // jump offset 
                bitset<8> jump = bitset<8> (state.ID.instruction.to_string().substr(8, 8));
                if(!isBusyRegister[R.to_ulong()])
                {
                    controlInstructions++;
                    // if 0 
                    if(reg.readData(R).to_ulong() == 0)
                    {
                        // update PC value accordingly 
                        if(jump[7])
                        {
                            state.IF.PC = state.IF.PC.to_ulong() + ((jump.to_ulong()-256)<<1);
                        }
                        else
                        {
                            state.IF.PC = state.IF.PC.to_ulong() + (jump.to_ulong()<<1);
                        }
                    }
                fetchPaused = false;
                exPaused = false;
                }
                else
                {
                    totalStalls++;
                    controlStalls++;
                    fetchPaused = true;
                    exPaused = true;
                }
                newState.EX.run = false;
            }
        }
        // --------------------------------------------------------------------
        //                          ID Stage {END}
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        //                          IF Stage {START}
        // --------------------------------------------------------------------
        newState.ID.run = state.IF.run;
        if(state.IF.run && !fetchPaused)
        {
            totalInstructions++;    // increment counter 
            // read instruction form iCache using PC
            newState.ID.instruction = iCache.readInstruction(state.IF.PC);

            // increment PC
            newState.IF.PC = state.IF.PC.to_ulong()+2;
            
            // if instruction is HALT
            if(newState.ID.instruction.to_string().substr(0,4) == "1111")
            {
                haltInstructions++;
                newState.IF.run = 0;
                newState.ID.run = 0;
            }
        }
        // --------------------------------------------------------------------
        //                          IF Stage {END}
        // --------------------------------------------------------------------

        if(!state.IF.run && !state.ID.run && !state.EX.run && !state.MEM.run && !state.WB.run)
        {
            break;
        }

        state = newState;
        cycle++;
    }
    // update DCache file 
    dCache.createResultFile();
    cout << cycle << endl;
    cpi = ((float)cycle/totalInstructions);

    // result file for counters  
    ofstream Output;
    Output.open("Output.txt");
    Output << "Total number of instructions executed: " << totalInstructions << endl;
    Output << "Number of instructions in each class" << endl;
    Output << "Arithmetic instructions              : " << arithmaticInstructions << endl;
    Output << "Logical instructions                 : " << logicalInstructions << endl;
    Output << "Data instructions                    : " << dataInstructions << endl;
    Output << "Control instructions                 : " << controlInstructions << endl;
    Output << "Halt instructions                    : " << haltInstructions << endl;
    Output << "Cycles Per Instruction               : " << cpi << endl;
    Output << "Total number of stalls               : " << totalStalls << endl;
    Output << "Data stalls (RAW)                    : " << dataStalls << endl;
    Output << "Control stalls                       : " << controlStalls << endl;
    Output.close();
    return 0;
}
