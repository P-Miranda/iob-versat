#include "versat.hpp"
#if nMEM > 0

versat_t CMem::read(uint32_t addr)
{
    if (addr >= MEM_SIZE)
    {
        printf("Invalid READ MEM ADDR=%u\n", addr);
        print_versat_mems();
        printf("VERSAT MEMS OUTPUTED AS FILE\n");
        print_versat_info();
        printf("VERSAT INFO OUTPUTED AS FILE\n");
        exit(0);
        //return 0;
    }
    return data[addr];
}
void CMem::write(uint32_t addr, versat_t data_in)
{
    if (addr >= MEM_SIZE)
    {
        printf("Invalid WRITE MEM ADDR=%u\n", addr);
        exit(0);
    }
    else
    {
        data[addr] = data_in;
    }
}
CMemPort::CMemPort() {}
CMemPort::CMemPort(int versat_base, int i, int offset)
{
    this->versat_base = versat_base;
    this->mem_base = i;
    this->data_base = offset;
    this->my_mem = &versat_mem[versat_base][i];
    this->iter = 0;
    this->per = 0;
    this->duty = 0;
    this->sel = 0;
    this->start = 0;
    this->shift = 0;
    this->incr = 0;
    this->delay = 0;
    this->in_wr = 0;
    this->rvrs = 0;
    this->ext = 0;
    this->iter2 = 0;
    this->per2 = 0;
    this->shift2 = 0;
    this->incr2 = 0;
}
void CMemPort::reset()
{
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    run_delay = 0;
}
void CMemPort::update_shadow_reg_MEM()
{
    if (data_base == 1)
    {
        shadow_reg[versat_base].memB[mem_base].iter = conf[versat_base].memB[mem_base].iter;
        shadow_reg[versat_base].memB[mem_base].start = conf[versat_base].memB[mem_base].start;
        shadow_reg[versat_base].memB[mem_base].per = conf[versat_base].memB[mem_base].per;
        shadow_reg[versat_base].memB[mem_base].duty = conf[versat_base].memB[mem_base].duty;
        shadow_reg[versat_base].memB[mem_base].sel = conf[versat_base].memB[mem_base].sel;
        shadow_reg[versat_base].memB[mem_base].shift = conf[versat_base].memB[mem_base].shift;
        shadow_reg[versat_base].memB[mem_base].incr = conf[versat_base].memB[mem_base].incr;
        shadow_reg[versat_base].memB[mem_base].delay = conf[versat_base].memB[mem_base].delay;
        shadow_reg[versat_base].memB[mem_base].ext = conf[versat_base].memB[mem_base].ext;
        shadow_reg[versat_base].memB[mem_base].in_wr = conf[versat_base].memB[mem_base].in_wr;
        shadow_reg[versat_base].memB[mem_base].rvrs = conf[versat_base].memB[mem_base].rvrs;
        shadow_reg[versat_base].memB[mem_base].iter2 = conf[versat_base].memB[mem_base].iter2;
        shadow_reg[versat_base].memB[mem_base].per2 = conf[versat_base].memB[mem_base].per2;
        shadow_reg[versat_base].memB[mem_base].shift2 = conf[versat_base].memB[mem_base].shift2;
        shadow_reg[versat_base].memB[mem_base].incr2 = conf[versat_base].memB[mem_base].incr2;
    }
    else
    {
        shadow_reg[versat_base].memA[mem_base].iter = conf[versat_base].memA[mem_base].iter;
        shadow_reg[versat_base].memA[mem_base].start = conf[versat_base].memA[mem_base].start;
        shadow_reg[versat_base].memA[mem_base].per = conf[versat_base].memA[mem_base].per;
        shadow_reg[versat_base].memA[mem_base].duty = conf[versat_base].memA[mem_base].duty;
        shadow_reg[versat_base].memA[mem_base].sel = conf[versat_base].memA[mem_base].sel;
        shadow_reg[versat_base].memA[mem_base].shift = conf[versat_base].memA[mem_base].shift;
        shadow_reg[versat_base].memA[mem_base].incr = conf[versat_base].memA[mem_base].incr;
        shadow_reg[versat_base].memA[mem_base].delay = conf[versat_base].memA[mem_base].delay;
        shadow_reg[versat_base].memA[mem_base].ext = conf[versat_base].memA[mem_base].ext;
        shadow_reg[versat_base].memA[mem_base].in_wr = conf[versat_base].memA[mem_base].in_wr;
        shadow_reg[versat_base].memA[mem_base].rvrs = conf[versat_base].memA[mem_base].rvrs;
        shadow_reg[versat_base].memA[mem_base].iter2 = conf[versat_base].memA[mem_base].iter2;
        shadow_reg[versat_base].memA[mem_base].per2 = conf[versat_base].memA[mem_base].per2;
        shadow_reg[versat_base].memA[mem_base].shift2 = conf[versat_base].memA[mem_base].shift2;
        shadow_reg[versat_base].memA[mem_base].incr2 = conf[versat_base].memA[mem_base].incr2;
    }
}
void CMemPort::start_run()
{

    done = 0;
    pos = start;
    //set run_delay
    if (data_base == 1)
    {
        run_delay = shadow_reg[versat_base].memB[mem_base].delay;
    }
    else
    {
        run_delay = shadow_reg[versat_base].memA[mem_base].delay;
    }
}
void CMemPort::update()
{
    int i = 0;
    //check for delay
    if (run_delay > 0)
    {
        run_delay--;
    }
    else
    {

        //update databus
        if (data_base == 0)
        {
            stage[versat_base].databus[sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
            //special case for stage 0
            if (versat_base == 0)
            {
                //2nd copy at the end of global databus
                global_databus[nSTAGE * N + sMEMA[mem_base]] = output_port[MEMP_LAT - 1];
            }
        }
        else
        {
            stage[versat_base].databus[sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
            //special case for stage 0
            if (versat_base == 0)
            {
                //2nd copy at the end of global databus
                global_databus[nSTAGE * N + sMEMB[mem_base]] = output_port[MEMP_LAT - 1];
            }
        }

        //trickle down all outputs in buffer
        for (i = 1; i < MEMP_LAT; i++)
        {
            output_port[i] = output_port[i - 1];
        }
        //insert new output
        output_port[0] = out; //TO DO: change according to output()
    }
}

int CMemPort::AGU()
{
    uint32_t aux = 0;
    while (i < iter2)
    {
        while (j < per2)
        {
            while (k < iter)
            {
                while (l < per)
                {
                    l++;
                    aux = pos;
                    pos += incr;
                    return aux;
                }
                l = 0;
                pos += shift;
                k++;
            }
            k = 0;
            pos += incr2;
            j++;
        }
        pos += shift2;
        j = 0;
        i++;
    }
    done = 1;
    pos = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    return 0;
}

versat_t CMemPort::output()
{
    if (run_delay > 0)
        return 0;

    int addr = 0;
    if (done == 0)
        addr = AGU();
    else
    {
        out = 0;
        return 0;
    }

    versat_t data;
    if (in_wr)
    {
        if (data_base == 0)
        {
            write(addr, stage[versat_base].databus[shadow_reg[versat_base].memA[mem_base].sel]);
            out = stage[versat_base].databus[shadow_reg[versat_base].memA[mem_base].sel];
        }
        else
        {
            write(addr, stage[versat_base].databus[shadow_reg[versat_base].memB[mem_base].sel]);
            out = stage[versat_base].databus[shadow_reg[versat_base].memB[mem_base].sel];
        }
    }
    else
    {
        data = read(addr);
        out = data;
    }
    return 0;
}

void CMemPort::writeConf()
{
    if (data_base == 1)
    {
        conf[versat_base].memB[mem_base].iter = iter;
        conf[versat_base].memB[mem_base].start = start;
        conf[versat_base].memB[mem_base].per = per;
        conf[versat_base].memB[mem_base].duty = duty;
        conf[versat_base].memB[mem_base].sel = sel;
        conf[versat_base].memB[mem_base].shift = shift;
        conf[versat_base].memB[mem_base].incr = incr;
        conf[versat_base].memB[mem_base].delay = delay;
        conf[versat_base].memB[mem_base].ext = ext;
        conf[versat_base].memB[mem_base].in_wr = in_wr;
        conf[versat_base].memB[mem_base].rvrs = rvrs;
        conf[versat_base].memB[mem_base].iter2 = iter2;
        conf[versat_base].memB[mem_base].per2 = per2;
        conf[versat_base].memB[mem_base].shift2 = shift2;
        conf[versat_base].memB[mem_base].incr2 = incr2;
    }
    else
    {
        conf[versat_base].memA[mem_base].iter = iter;
        conf[versat_base].memA[mem_base].start = start;
        conf[versat_base].memA[mem_base].per = per;
        conf[versat_base].memA[mem_base].duty = duty;
        conf[versat_base].memA[mem_base].sel = sel;
        conf[versat_base].memA[mem_base].shift = shift;
        conf[versat_base].memA[mem_base].incr = incr;
        conf[versat_base].memA[mem_base].delay = delay;
        conf[versat_base].memA[mem_base].ext = ext;
        conf[versat_base].memA[mem_base].in_wr = in_wr;
        conf[versat_base].memA[mem_base].rvrs = rvrs;
        conf[versat_base].memA[mem_base].iter2 = iter2;
        conf[versat_base].memA[mem_base].per2 = per2;
        conf[versat_base].memA[mem_base].shift2 = shift2;
        conf[versat_base].memA[mem_base].incr2 = incr2;
    }
}

void CMemPort::setIter(int iter)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].iter = iter;
    else
    {
        conf[versat_base].memA[mem_base].iter = iter;
    }
    this->iter = iter;
}

void CMemPort::setPer(int per)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].per = per;
    else
    {
        conf[versat_base].memA[mem_base].per = per;
    }
    this->per = per;
}

void CMemPort::setDuty(int duty)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].duty = duty;
    else
    {
        conf[versat_base].memA[mem_base].duty = duty;
    }
    this->duty = duty;
}

void CMemPort::setSel(int sel)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].sel = sel;
    else
    {
        conf[versat_base].memA[mem_base].sel = sel;
    }
    this->sel = sel;
}

void CMemPort::setStart(int start)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].start = start;
    else
    {
        conf[versat_base].memA[mem_base].start = start;
    }
    this->start = start;
}
void CMemPort::setIncr(int incr)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].incr = incr;
    else
    {
        conf[versat_base].memA[mem_base].incr = incr;
    }
    this->incr = incr;
}

void CMemPort::setShift(int shift)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].shift = shift;
    else
    {
        conf[versat_base].memA[mem_base].shift = shift;
    }
    this->shift = shift;
}

void CMemPort::setDelay(int delay)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].delay = delay;
    else
    {
        conf[versat_base].memA[mem_base].delay = delay;
    }
    this->delay = delay;
}

void CMemPort::setExt(int ext)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].ext = ext;
    else
    {
        conf[versat_base].memA[mem_base].ext = ext;
    }
    this->ext = ext;
}

void CMemPort::setRvrs(int rvrs)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].rvrs = rvrs;
    else
    {
        conf[versat_base].memA[mem_base].rvrs = rvrs;
    }
    this->rvrs = rvrs;
}

void CMemPort::setInWr(int in_wr)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].in_wr = in_wr;
    else
    {
        conf[versat_base].memA[mem_base].in_wr = in_wr;
    }
    this->in_wr = in_wr;
}

void CMemPort::setIter2(int iter2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].iter2 = iter2;
    else
    {
        conf[versat_base].memA[mem_base].iter2 = iter2;
    }
    this->iter2 = iter2;
}

void CMemPort::setPer2(int per2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].per2 = per2;
    else
    {
        conf[versat_base].memA[mem_base].per2 = per2;
    }
    this->per2 = per2;
}

void CMemPort::setIncr2(int incr)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].incr2 = incr2;
    else
    {
        conf[versat_base].memA[mem_base].incr2 = incr2;
    }
    this->incr2 = incr2;
}

void CMemPort::setShift2(int shift2)
{
    if (data_base == 1)
        conf[versat_base].memB[mem_base].shift2 = shift2;
    else
    {
        conf[versat_base].memA[mem_base].shift2 = shift2;
    }
    this->shift2 = shift2;
}

void CMemPort::write(int addr, int val)
{
    //MEMSET(versat_base, (this->data_base + addr), val);
    if (done == 0)
        my_mem->write(addr, val);
}

int CMemPort::read(int addr)
{
    //return MEMGET(versat_base, (this->data_base + addr));
    if (done == 0)
        return my_mem->read(addr);
    else
    {
        return 0;
    }
}
string CMemPort::info()
{
    string ver = "mem";
    if (data_base == 0)
        ver += "A[" + to_string(mem_base) + "]\n";
    else
    {
        ver += "B[" + to_string(mem_base) + "]\n";
    }
    ver += "Start=    " + to_string(start) + "\n";
    ver += "Iter=     " + to_string(iter) + "\n";
    ver += "Per =     " + to_string(per) + "\n";
    ver += "Incr=     " + to_string(incr) + "\n";
    ver += "Delay=    " + to_string(delay) + "\n";
    ver += "Shift=    " + to_string(shift) + "\n";
    ver += "Duty=     " + to_string(duty) + "\n";
    ver += "Sel=      " + to_string(sel) + "\n";
    ver += "Ext=      " + to_string(ext) + "\n";
    ver += "Rvrs =    " + to_string(rvrs) + "\n";
    ver += "InWr=     " + to_string(in_wr) + "\n";
    ver += "Iter2=    " + to_string(iter2) + "\n";
    ver += "Per2 =    " + to_string(per2) + "\n";
    ver += "Shift2=   " + to_string(shift2) + "\n";
    ver += "Incr2=    " + to_string(incr2) + "\n";
    ver += "Done=     " + to_string(done) + "\n";
    ver += "\n";

    return ver;
}
#endif