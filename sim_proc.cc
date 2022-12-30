#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>
#include "sim_proc.h"


#include<string>
#include<map>

using namespace std;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int rtl;
struct instruction
{
    int ins_nu;
    int fe_s;
    int fe_t;
    int de_s;
    int de_t;
    int rn_s;
    int rn_t;
    int rr_s;
    int rr_t;
    int di_s;
    int di_t;
    int is_s;
    int is_t;
    int ex_s;
    int ex_t;
    int wb_s;
    int wb_t;
    int rt_s;
    int rt_t;
    int issue_flag;
    unsigned long int pc; 
    int op;
    int dest;
    int r_dest;
    int sc1;
    int r_sc1;
    string r_sc1_source;
    int rw1_ready;
    int r_sc1_ready;
    int sc2;
    int r_sc2;
    string r_sc2_source;
    int rw2_ready;
    int r_sc2_ready;
    int ready_for_rt;
    int ready_for_wb;
    int ex_cycle;
    int ready_for_ex;
    int placed_in_iq;
    int renamed_once;
    int ready_for_is;
    int ready_for_di;
    int ready_for_rr;
    int ready_for_rn;
    int ready_for_de;
    int ready_for_fe;
    int fti_fe;
    int fti_de;
    int fti_rn;
    int fti_rr;
    int fti_di;
    int fti_is;
    int fti_ex;
    int fti_wb;
    int fti_rt;
    
};

struct reorder_buffer
{
    int valid;
    string value;
    int dst;
    int ready;
    int head;
    int tail;
    unsigned long int pc;
};

struct rmt
{
    int valid;
    int tag;
};

struct dispatch_queue
{
    int valid;
    int dst;
    int rs1_ready;
    string rs1_source;
    int rs1_value;
    int rs2_ready;
    string rs2_source;
    int rs2_value;
    int ins_num;
};

struct reg_read_queue
{
    int valid;
    int dst;
    int rs1_ready;
    string rs1_source;
    int rs1_value;
    int rs2_ready;
    string rs2_source;
    int rs2_value;
    int ins_num;
};

struct rename_queue
{
    int valid;
    int dst;
    int rs1_ready;
    string rs1_source;
    int rs1_value;
    int rs2_ready;
    string rs2_source;
    int rs2_value;
    int ins_num;
};

struct re
{
    int valid;
    int dst;
    int rs1_ready;
    string rs1_source;
    int rs1_value;
    int rs2_ready;
    string rs2_source;
    int rs2_value;
    int ins_num;
};

struct issue_queue
{
    int valid;
    int dst;
    int rs1_ready;
    string rs1_source;
    int rs1_value;
    int rs2_ready;
    string rs2_source;
    int rs2_value;
    int ins_num;
};


struct instruction ins[10000];
struct rmt r[67];
int current_cycle=-1;
int execute_buffer_full=0;
int stall_pipeline=0;
int stall_dispatch=0;
int dispatch_buffer_count=0;
int reg_read_buffer_count=0;
int rename_buffer_count=0;
int decode_buffer_count=0;
int stall_iq=0;
int retired_instructions_post_stall=0;
int stall_retire;


void display_rob(reorder_buffer* rob,int rob_size)
{
    cout<<"\n\n\n Current Cycle : "<<current_cycle<<"===============ROB Contents==================";
    for(int i=0;i<rob_size;i++)
    {
       cout<<"\nrob "<<i<<" : "<<"Valid : "<<rob[i].valid<<"   ||   Dest :  "<<rob[i].dst<<"   ||   ready : "<<rob[i].ready<<"   ||   PC : "<<rob[i].pc;
       if(rob[i].head==1)
       {
        cout<<"------>H";
       }
       if(rob[i].tail==1)
       {
        cout<<"------>T";
       }
    }
    
}


void display_rmt()
{
    cout<<"\n\n\n===============RMT Contents==================";
    for(int i=0;i<67;i++)
    {
       cout<<"\nr"<<i<<" : "<<"Valid : "<<r[i].valid<<"   ||   Tag : "<<r[i].tag;
    }
    
}

void display_iq(issue_queue* iq,int iq_size)
{
    cout<<"\n\n\n Current Cycle : "<<current_cycle<<"===============IQ Contents==================";
    for(int i=0;i<iq_size;i++)
    {
       cout<<"\n"<<i<<" : "<<"Valid : "<<iq[i].valid<<"   ||   dest : rob "<<iq[i].dst<<"   ||   rs1_ready : "<<iq[i].rs1_ready<<"   ||   rs1_value : "<<iq[i].rs1_source<< iq[i].rs1_value<<"   ||   rs2_ready : "<<iq[i].rs2_ready<<"   ||   rs2_value : "<<iq[i].rs2_source<< iq[i].rs2_value<<"   ||   Ins : "<<iq[i].ins_num;
    }
    
}

int rob_full(reorder_buffer* rob,int rob_size,int width)
{
    int count=0;
    for(int i=0;i<rob_size;i++)
    {
       if(rob[i].valid==0)
       {
        count++;
       }
    }
    if(count<width)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    
}

int rob_spaces(reorder_buffer* rob,int rob_size)
{
    int count=0;
    for(int i=0;i<rob_size;i++)
    {
       if(rob[i].valid==0)
       {
        count++;
       }
    }
     return count;

    
}

void Retire(instruction* ins,reorder_buffer* rob,issue_queue* iq,int width,int rob_size,int iq_size)
{
    int retire_count=0;
    int pos=0;
    int empty_spaces_rob=0;
    int empty_spaces_iq=0;
    //if(stall_pipeline==0)
   // {
        //retired_instructions_post_stall=0;
    //}
    if(current_cycle==20||current_cycle==21||current_cycle==22)
    {
       // cout<<"\nBefore Retire cycle  : "<<current_cycle;
       // display_rob(rob,rob_size);
    }
    for(int i=0;i<10000;i++)
    {
        if(ins[i].ready_for_rt==1)
        {
            if(ins[i].fti_rt==0)
            {
                ins[i].rt_s=current_cycle;
                ins[i].fti_rt=1;
            }
            //cout<<"\n**********  Inside Retire Instruction "<<i<<"  ***************";
            if(retire_count<width)
            {
                if(rob[ins[i].r_dest].head==1)
                {
                    //cout<<"\n========Ready for Retirement==========";
                    retire_count++;
                    rob[ins[i].r_dest].head=0;
                    pos=rob[ins[i].r_dest].dst;
                    if(r[pos].tag==ins[i].r_dest)
                    {
                        r[pos].tag=-1;
                        r[pos].valid=0;
                    }
                    rob[ins[i].r_dest].valid=0;
                    rob[ins[i].r_dest].dst=-2;
                    rob[ins[i].r_dest].ready=0;
                    rob[ins[i].r_dest].pc=0;
                    if((ins[i].r_dest+1)==rob_size)
                    {
                        rob[0].head=1;
                    }
                    if((ins[i].r_dest+1)!=rob_size)
                    {
                        rob[ins[i].r_dest+1].head=1;
                    }
                    ins[i].ready_for_rt=2;
                   // if(stall_pipeline==1)
                   // {
                        //retired_instructions_post_stall++;
                   // }
                   // if(retired_instructions_post_stall==2)
                   // {
                       // stall_pipeline=0;
                   // }
                }
            }
            //display_rob();
            ins[i].rt_t++;
            for(int n=0;n<rob_size;n++)
            {
                if(rob[n].valid==0)
                {
                    empty_spaces_rob++;
                }
            }
            if(empty_spaces_rob<width)
            {
                stall_pipeline=1;
            }
            else
            {
                stall_pipeline=0;
            }

            for(int g=0;g<iq_size;g++)
            {
                if(iq[g].valid==0)
                {
                    empty_spaces_iq++;
                }
            }
            if(empty_spaces_iq<width)
            {
                stall_iq=1;
                stall_retire=1;
                stall_dispatch=1;
            }
            else
            {
                stall_iq=0;
                stall_retire=0;
                stall_dispatch=0;
            }
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
            
        }
    }
    if(current_cycle==20||current_cycle==21||current_cycle==22)
    {
       // cout<<"\nAfter Retire cycle  : "<<current_cycle;
       // display_rob(rob);
    }
    
}

void Writeback(instruction* ins,reorder_buffer* rob,int width)
{
    int write_back_count=0;
    for(int i=0;i<10000;i++)
    {
        if((ins[i].ready_for_wb==1)&&(write_back_count<(width*5)))
        {
            //cout<<"\n**********  Inside Writeback Instruction "<<i<<"  ***************";
            if(ins[i].fti_wb==0)
            {
                ins[i].wb_s=current_cycle;
                ins[i].fti_wb=1;
            }
            rob[ins[i].r_dest].ready=1;
            ins[i].ready_for_wb=2;
            ins[i].ready_for_rt=1;
            ins[i].wb_t++;
            write_back_count++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }
    
}


void Execute(instruction* ins,issue_queue* iq,dispatch_queue* dq,int width,int iq_size,rename_queue* rn)
{
    int count=0;
    //int count2=0;
    for(int i=0;i<10000;i++)
    {
        if(ins[i].ready_for_ex==1)
        {
            //count++;       
            if(ins[i].fti_ex==0)
            {
                ins[i].ex_s=current_cycle;
                ins[i].fti_ex=1;
            }
            //cout<<"\n**********  Inside Execute Instruction "<<i<<"  ***************";
            //cout<<"\n********** Line "<<i<<" ex cycle : "<<ins[i].ex_cycle;
            
            if(ins[i].ex_cycle==1)
            {
                for(int n=0;n<width;n++)
                {
                    if((ins[i].r_dest)==(dq[n].rs1_value))  
                    {
                        ins[dq[n].ins_num].r_sc1_ready=1;
                    }
                    if((ins[i].r_dest)==(dq[n].rs2_value))  
                    {
                        ins[dq[n].ins_num].r_sc2_ready=1;
                    }
                }

                /*for(int a=0;a<width;a++)
                {
                    if((ins[i].r_dest)==(rn[a].rs1_value))  
                    {
                        ins[rn[a].ins_num].rw1_ready=1;
                    }
                    if((ins[i].r_dest)==(rn[a].rs2_value))  
                    {
                        ins[rn[a].ins_num].rw2_ready=1;
                    }
                }*/

                for(int j=0;j<iq_size;j++)
                {
                    if(iq[j].valid==1)
                    {
                        if(iq[j].rs1_ready==0)
                        {
                            if(ins[i].r_dest==iq[j].rs1_value)
                            {
                                iq[j].rs1_ready=2;//wake up
                            }
                        }

                        if(iq[j].rs2_ready==0)
                        {
                            if(ins[i].r_dest==iq[j].rs2_value)
                            {
                                iq[j].rs2_ready=2;//wake up
                            }
                        }
                    }
                }
            }
            ins[i].ex_cycle=ins[i].ex_cycle - 1;
            //cout<<"\n***** sub Line "<<i<<" ex cycle : "<<ins[i].ex_cycle;
            if(ins[i].ex_cycle==0)
            {
                ins[i].ready_for_ex=2;
                ins[i].ready_for_wb=1;
            }
            else if(ins[i].ex_cycle!=0)
            {
                count++;
            }
            ins[i].ex_t++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }
    if(count>=(width*5))
    {
        execute_buffer_full=5;
        //cout<<"**********++++++++++++BUFFER FULL++++++++++++++*********************  : "<<count;
    }
    else
    {
        execute_buffer_full=4;
        //cout<<"**********++++++++++++BUFFER NOT FULL++++++++++++++*********************   : "<<count;
    }
    
}

void Issue(instruction* ins,issue_queue* iq,reorder_buffer* rob,int width,int iq_size)
{
    //0 means not ready
    //1 means ready
    //2 means waking up
    int issue_count=0;
    
    if(current_cycle==414||current_cycle==415||current_cycle==416||current_cycle==417||current_cycle==410||current_cycle==409||current_cycle==408||current_cycle==407||current_cycle==406||current_cycle==405||current_cycle==404||current_cycle==403)
    {
        //cout<<"\n=======================BEFORE ISSUE cycle : "<<current_cycle;
        //display_iq(iq,iq_size);
    }
    if(current_cycle==338)
    {
       // display_rob();
        //display_rmt();
    }
    for(int i=0;i<10000;i++)
    {
        if(ins[i].ready_for_is==1)
        {
            if(current_cycle==252)
            {
                //cout<<"\nHi : "<<i;
               // cout<<"\nEx buffer : "<<execute_buffer_full;
            }
            if(ins[i].fti_is==0)
            {
                ins[i].is_s=current_cycle;
                ins[i].fti_is=1;
            }
            //cout<<"\n**********  Inside Issue Instruction "<<i<<"  ***************";
            if(ins[i].placed_in_iq==0)
            {
                //cout << "\n Placing inside issue queue.....";
                for (int j = 0; j < iq_size; j++)
                {
                    if (iq[j].valid == 0)
                    {
                        if(current_cycle==252)
                        {
                            //cout<<"\n for instruction "<<i<<", Place found in iq ... : "<<j;
                        }
                        iq[j].valid = 1;
                        iq[j].dst = ins[i].r_dest;
                        if((ins[i].r_dest==18)&&(i<1500))
                        {
                            rtl=i;
                        }
                        iq[j].rs1_ready = ins[i].r_sc1_ready;
                        iq[j].rs1_source=ins[i].r_sc1_source;
                        iq[j].rs1_value=ins[i].r_sc1;
                        iq[j].rs2_ready = ins[i].r_sc2_ready;
                        iq[j].rs2_source=ins[i].r_sc2_source;
                        iq[j].rs2_value=ins[i].r_sc2;
                        iq[j].ins_num = i;
                        break;
                    }
                }
                ins[i].placed_in_iq=1;
            }


            for(int k=0;k<iq_size;k++)
            {
                if(iq[k].ins_num==i)
                {
                    if(ins[i].r_sc1_source=="rob")
                    {
                        if((rob[ins[i].r_sc1].ready==1)||(iq[k].rs1_ready==2)||(ins[i].r_sc1_ready==1))
                        {
                            iq[k].rs1_ready=1;
                        }
                    }
                    if(ins[i].r_sc2_source=="rob")
                    {
                        if((rob[ins[i].r_sc2].ready==1)||(iq[k].rs2_ready==2)||(ins[i].r_sc2_ready==1))
                        {
                            iq[k].rs2_ready=1;
                        }
                    }
                    if((iq[k].rs1_ready==1)&&(iq[k].rs2_ready==1)&&(execute_buffer_full==4))
                    {
                        if(issue_count<width)
                        {
                            issue_count++;
                            ins[i].ready_for_is=2;
                            ins[i].ready_for_ex=1;
                            iq[k].valid=0;
                        }
                    }
                    
                }    
            }
            if(current_cycle==414||current_cycle==415||current_cycle==416||current_cycle==417)
            {
               // display_iq(iq,iq_size);
            }
            ins[i].is_t++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }

   
    if(current_cycle==414||current_cycle==415||current_cycle==402||current_cycle==401||current_cycle==410||current_cycle==409||current_cycle==408||current_cycle==407||current_cycle==406||current_cycle==405||current_cycle==404||current_cycle==403)
    {
         //cout<<"\n=======================After ISSUE=====================cycle : "<<current_cycle;
         //display_iq(iq,iq_size);
    }
    if(current_cycle==338)
    {
        //display_rob();
        //display_rmt();
    }
    
}

void Dispatch(instruction* ins,dispatch_queue* dq,reorder_buffer* rob,issue_queue* iq,int width,int iq_size)
{
    if(current_cycle==31||current_cycle==32||current_cycle==33||current_cycle==34)
    {
         //cout<<"\n=======================before dispatch=====================cycle : "<<current_cycle;
         //display_iq(iq,iq_size);
    }
    int dispatch_count=0;
    dispatch_buffer_count=0;
    for(int i=0;i<10000;i++)
    {
        if((ins[i].ready_for_di==1)&&(dispatch_count<width))
        {
            int empty_spaces_iq=0;
            if(ins[i].fti_di==0)
            {
                ins[i].di_s=current_cycle;
                ins[i].fti_di=1;
            }
            //cout<<"\n**********  Inside Dispatch Instruction "<<i<<"  ***************";
            dq[dispatch_count].valid = 1;
            dq[dispatch_count].dst = ins[i].r_dest;
            dq[dispatch_count].rs1_ready = ins[i].r_sc1_ready;
            dq[dispatch_count].rs1_source = ins[i].r_sc1_source;
            dq[dispatch_count].rs1_value = ins[i].r_sc1;
            dq[dispatch_count].rs2_ready = ins[i].r_sc2_ready;
            dq[dispatch_count].rs2_source = ins[i].r_sc2_source;
            dq[dispatch_count].rs2_value = ins[i].r_sc2;
            dq[dispatch_count].ins_num = i;
            if((rob[ins[i].r_sc1].ready==1))
            {
                    ins[i].r_sc1_ready=1;
            }
            if((rob[ins[i].r_sc2].ready==1))
            {
                ins[i].r_sc2_ready=1;
            }
            for(int m=0;m<iq_size;m++)
            {
                if(iq[m].valid==0)
                {
                    empty_spaces_iq++;
                }
            }
            if(empty_spaces_iq>=(width))
            {
                ins[i].ready_for_di=2;
                ins[i].ready_for_is=1;
            }
            else
            {
                dispatch_buffer_count++;
                stall_pipeline=1;
                stall_dispatch=1;
            }
            ins[i].di_t++;
            dispatch_count++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }
    if(current_cycle==110||current_cycle==111||current_cycle==112||current_cycle==113||current_cycle==114||current_cycle==115)
    {
         //cout<<"\n=======================After Dispatch=====================cycle : "<<current_cycle;
         if(stall_dispatch==1)
         {
            //cout<<"\n Dispatch is causing stall";
         }
         if((stall_pipeline==1)&&(stall_dispatch==0))
         {
            //cout<<"\n Dispatch is not causing stall.....";
         }
         //display_iq(iq,iq_size);
         //display_iq(iq,iq_size);
    }
    
}

void Reg_Read(instruction* ins,reorder_buffer* rob,reg_read_queue* rr,int width)
{
    int reg_read_count=0;
    reg_read_buffer_count=0;
    for(int i=0;i<10000;i++)
    {
        if((ins[i].ready_for_rr==1)&&(reg_read_count<width))
        {
            //cout<<"\n**********  Inside Register Read Instruction "<<i<<"  ***************";
            if(ins[i].fti_rr==0)
            {
                ins[i].rr_s=current_cycle;
                ins[i].fti_rr=1;
            }
            rr[reg_read_count].valid = 1;
            rr[reg_read_count].dst = ins[i].r_dest;
            rr[reg_read_count].rs1_ready = ins[i].r_sc1_ready;
            rr[reg_read_count].rs1_source = ins[i].r_sc1_source;
            rr[reg_read_count].rs1_value = ins[i].r_sc1;
            rr[reg_read_count].rs2_ready = ins[i].r_sc2_ready;
            rr[reg_read_count].rs2_source = ins[i].r_sc2_source;
            rr[reg_read_count].rs2_value = ins[i].r_sc2;
            rr[reg_read_count].ins_num = i;
            if((rob[ins[i].r_sc1].ready==1))
            {
                    ins[i].r_sc1_ready=1;
            }
            if((rob[ins[i].r_sc2].ready==1))
            {
                ins[i].r_sc2_ready=1;
            }
            if((dispatch_buffer_count==0))
            {
                ins[i].ready_for_rr=2;
                ins[i].ready_for_di=1;
            }
            else
            {
                reg_read_buffer_count++;
            }
            ins[i].rr_t++;
            reg_read_count++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }
    
}



void Rename(instruction* ins,reorder_buffer* rob,int width,int rob_size,rename_queue* rn)
{
    int rename_count=0;
    int stall_rename=0;
    int sc1_new=0;
    int sc2_new=0;
    string r1="";
    string r2="";
    int empty_spaces_rn=0;
    int flag=0;
    rename_buffer_count=0;

    /*if(current_cycle==399||current_cycle==400||current_cycle==401||current_cycle==402)
    {
        cout<<"\nBefore rename, cycle : "<<current_cycle;
        cout<<"\n Stall : "<<stall_pipeline;
        int y=rob_full(rob,rob_size,width);
        cout<<"\n Rob full : "<<y;
        display_rob(rob,rob_size);
        //display_rmt();
    } */

    for(int m=0;m<rob_size;m++)
    {
        if(rob[m].valid==0)
        {
            empty_spaces_rn++;
        }
    }
    if(empty_spaces_rn < width)
    {
        stall_pipeline=1;
        flag=1;
    }

    for(int i=0;i<10000;i++)
    {
        if((ins[i].ready_for_rn==1)&&(rename_count<width))
        {
            if(ins[i].fti_rn==0)
            {
                ins[i].rn_s=current_cycle;
                ins[i].fti_rn=1;
            }
             if(current_cycle==399||current_cycle==400||current_cycle==401||current_cycle==402)
             {
                //cout<<"\n**********  Inside Rename Instruction "<<i<<"  ***************";
                //cout<<"\n rename count : "<<rename_count;
             }
           //cout<<"\n rename count : "<<rename_count;
            

            //cout<<"\n=======Moving Tail and updating RMT============";
            for(int j=0;j<rob_size;j++)
            {
                if(rob[j].tail==1)
                {
                    if((rob[j].head==1)&&(current_cycle!=2))
                    {
                        stall_rename=1;
                        stall_pipeline=1;
                        //cout<<"\n==============================Pipeline Stalled==================================";
                        break;
                    }
                    int x=rob_spaces(rob,rob_size);
                    if(current_cycle==399||current_cycle==400||current_cycle==401||current_cycle==402)
                    {
                      //cout<<"     flag : "<<flag;
                    }
                    if((stall_pipeline==1)&&(flag==1))
                    {
                        if(current_cycle==110||current_cycle==111||current_cycle==112||current_cycle==113||current_cycle==114||current_cycle==115)
                        {
                            //cout<<"\n    reg_read_buffer : "<<reg_read_buffer_count;
                        }
                        //cout<<"\n..............Hi....... X+ rename_count"<<x+rename_count;
                        if(reg_read_buffer_count>=width)
                        {
                            stall_rename=1;
                            break;
                        }

                    }


                    if(flag==0)
                    {
                    if(ins[i].renamed_once==0)
                    {

                        /*if((rob[j].head!=1)&&(stall_pipeline==1))
                        {
                            stall_pipeline=0;
                        }*/
                        // cout<<"\nTail found....";
                        rob[j].tail = 0;
                        rob[j].valid = 1;
                        rob[j].dst = ins[i].dest;
                        /*if(ins[i].dest!=-1)
                        {
                            r[ins[i].dest].valid=1; //Mapping it in rmt
                            r[ins[i].dest].tag=j; //mapping tag in rmt
                        }*/
                        rob[j].pc = ins[j].pc;
                        if ((j + 1) == rob_size)
                        {
                            rob[0].tail = 1;
                        }
                        else if ((j + 1) != rob_size)
                        {
                            rob[j + 1].tail = 1;
                        }
                        ins[i].r_dest = j;
                        // cout<<"\nInitial instruction              : "<<ins[i].dest<<","<<ins[i].sc1<<","<<ins[i].sc2;
                        if (ins[i].sc1 != -1)
                        {
                            if (r[ins[i].sc1].valid == 1)
                            {
                                sc1_new = r[ins[i].sc1].tag;
                                r1 = "rob";
                                if (rob[r[ins[i].sc1].tag].ready == 1)
                                {
                                    ins[i].r_sc1_ready = 1;
                                }
                            }
                            else
                            {
                                sc1_new = ins[i].sc1;
                                r1 = "r";
                                ins[i].r_sc1_ready = 1;
                            }
                        }
                        else if (ins[i].sc1 == -1)
                        {
                            ins[i].r_sc1_ready = 1;
                            sc1_new = ins[i].sc1;
                            r1 = "No";
                        }

                        if (ins[i].sc2 != -1)
                        {
                            if (r[ins[i].sc2].valid == 1)
                            {
                                sc2_new = r[ins[i].sc2].tag;
                                r2 = "rob";
                                if (rob[r[ins[i].sc2].tag].ready == 1)
                                {
                                    ins[i].r_sc2_ready = 1;
                                }
                        
                            }
                            else
                            {
                                sc2_new = ins[i].sc2;
                                r2 = "r";
                                ins[i].r_sc2_ready = 1;
                            }
                        }

                        else if (ins[i].sc2 == -1)
                        {
                            ins[i].r_sc2_ready = 1;
                            sc2_new = ins[i].sc2;
                            r2 = "No";
                        }
                        ins[i].r_sc1 = sc1_new;
                        ins[i].r_sc1_source = r1;
                        ins[i].r_sc2 = sc2_new;
                        ins[i].r_sc2_source = r2;
                        if (ins[i].dest != -1)
                        {
                            r[ins[i].dest].valid = 1; // Mapping it in rmt
                            r[ins[i].dest].tag = j;   // mapping tag in rmt
                        }
                        ins[i].renamed_once = 1;
                        if(current_cycle==399||current_cycle==400||current_cycle==401||current_cycle==402||current_cycle==403||current_cycle==404)
                        {
                            //cout<<"\nInstrcution "<<i<<" renamed once....."   ;
                        }
                        break;
                    }
                    }
                }
            }

           /* if((ins[i].renamed_once==1))
            {
                ins[i].ready_for_rn=2;
                ins[i].ready_for_rr=1;
            }*/
            /*if((stall_rename!=1)&&(stall_dispatch!=1))
            {
                ins[i].ready_for_rn=2;
                ins[i].ready_for_rr=1;
            }*/

            if((ins[i].renamed_once==1)&&(reg_read_buffer_count==0))
            {
                ins[i].ready_for_rn=2;
                ins[i].ready_for_rr=1;
            }
            else
            {
                rename_buffer_count++;
            
            }

            if((ins[i].renamed_once==1)&&(rob[ins[i].r_sc1].ready==1))
            {
                    ins[i].r_sc1_ready=1;
            }

        
            if((ins[i].renamed_once==1)&&(rob[ins[i].r_sc2].ready==1))
            {
                    ins[i].r_sc2_ready=1;
            }

            rn[rename_count].valid = 1;
            rn[rename_count].dst = ins[i].r_dest;
            rn[rename_count].rs1_ready = ins[i].rw1_ready;
            rn[rename_count].rs1_source = ins[i].r_sc1_source;
            rn[rename_count].rs1_value = ins[i].r_sc1;
            rn[rename_count].rs2_ready = ins[i].rw2_ready;
            rn[rename_count].rs2_source = ins[i].r_sc2_source;
            rn[rename_count].rs2_value = ins[i].r_sc2;
            rn[rename_count].ins_num = i;

            rename_count++;
            ins[i].rn_t++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
            /*if(current_cycle==153||current_cycle==154||current_cycle==155||current_cycle==152||current_cycle==151||current_cycle==150)
            {
                //display_rob();
                //display_rmt();
            }*/

        }
        
        
    }
        
   /* if(current_cycle==399||current_cycle==400||current_cycle==401||current_cycle==402)
    {
        cout<<"\nAfter rename, cycle : "<<current_cycle;
        cout<<"\n Stall : "<<stall_pipeline;
        int z=rob_full(rob,rob_size,width);
        cout<<"\n Rob full : "<<z;
        display_rob(rob,rob_size);
        //display_rmt();
    }*/
     
}

void Decode(instruction* ins,int width)
{
    int decode_count=0;
    decode_buffer_count=0;
    for(int i=0;i<10000;i++)
    {
        if((ins[i].ready_for_de==1)&&(decode_count < width))
        {
            //cout<<"\n**********  Inside Decode Instruction "<<i<<"  ***************";
            if(ins[i].fti_de==0)
            {
                ins[i].de_s=current_cycle;
                ins[i].fti_de=1;
            }

            if(rename_buffer_count==0)
            {
                ins[i].ready_for_de=2;
                ins[i].ready_for_rn=1;
            }
            else
            {
                decode_buffer_count++;
            }
            decode_count++;
            ins[i].de_t++;
            //cout<<"\n";
            //cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
        }
    }
    
}

void Fetch(instruction* ins,int width)
{
    if(current_cycle==111||current_cycle==112||current_cycle==113||current_cycle==114)
    {
         //cout<<"\n=======================before fetch=====================cycle : "<<current_cycle;
        // cout<<"\n=====================decode count   :  "<<decode_buffer_count;
         //display_iq(iq,iq_size);
    }
    int fetch_count=0;
    if(decode_buffer_count==0)
    {
        for (int i = 0; i < 10000; i++)
        {
            if ((ins[i].ready_for_fe == 1) && (fetch_count < width ))
            {
                //cout << "\n**********  Inside Fetch Instruction " << i << "  ***************";
                if (ins[i].fti_fe == 0)
                {
                    ins[i].fe_s = current_cycle;
                    ins[i].fti_fe = 1;
                }

                ins[i].ready_for_fe = 2;
                ins[i].ready_for_de = 1;
                fetch_count++;

                ins[i].fe_t++;
                // cout<<"\n";
                // cout<<ins[i].ins_nu<<" "<<"fu{"<<ins[i].op<<"} src{"<<ins[i].sc1<<","<<ins[i].sc2<<"} dst{"<<ins[i].dest<<"} FE{"<<ins[i].fe_s<<","<<ins[i].fe_t<<"} DE{"<<ins[i].de_s<<","<<ins[i].de_t<<"} RN{"<<ins[i].rn_s<<","<<ins[i].rn_t<<"} RR{"<<ins[i].rr_s<<","<<ins[i].rr_t<<"} DI{"<<ins[i].di_s<<","<<ins[i].di_t<<"} IS{"<<ins[i].is_s<<","<<ins[i].is_t<<"} EX{"<<ins[i].ex_s<<","<<ins[i].ex_t<<"} WB{"<<ins[i].wb_s<<","<<ins[i].wb_t<<"} RT{"<<ins[i].rt_s<<","<<ins[i].rt_t<<"}";
            }
        }
    }
    
}


int main(int argc, char *argv[])
{
    FILE *FP;                      // File handler
    char *trace_file;              // Variable that holds trace file name;
    proc_params params;            // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, desti, src1, src2; // Variables are read from trace file
    unsigned long int pci;          // Variable holds the pc read from input file
    int rob_size;
    int iq_size;
    int width;
    //string l; 

    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    params.rob_size = strtoul(argv[1], NULL, 10);
    params.iq_size = strtoul(argv[2], NULL, 10);
    params.width = strtoul(argv[3], NULL, 10);
    trace_file = argv[4];
    /*printf("rob_size:%lu "
           "iq_size:%lu "
           "width:%lu "
           "tracefile:%s\n",
           params.rob_size, params.iq_size, params.width, trace_file);*/
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    width=params.width;
    rob_size=params.rob_size;
    iq_size=params.iq_size;

    int i;
    struct reorder_buffer* rob=new reorder_buffer[rob_size];
    struct issue_queue* iq=new issue_queue[iq_size];
    struct dispatch_queue *dq=new dispatch_queue[width];
    struct reg_read_queue *rr=new reg_read_queue[width];
    struct rename_queue *rn=new rename_queue[width];

    for(i=0;i<rob_size;i++)
    {
        rob[i].valid=0;
        rob[i].value="";
        rob[i].dst=-2;
        rob[i].ready=0;
        if(i!=0)
        {
            rob[i].head=0;
            rob[i].tail=0;
        }
        else if(i==0)
        {
            rob[i].head=1;
            rob[i].tail=1;  
        }
       
    }


    for(i=0;i<67;i++)
    {   
        r[i].valid=0;
        r[i].tag=-1;    
    }


    for(i=0;i<iq_size;i++)
    {   
        iq[i].valid=0;
        iq[i].dst=-1;
        iq[i].rs1_ready=0;
        iq[i].rs1_source="";
        iq[i].rs1_value=-2;
        iq[i].rs2_ready=0;
        iq[i].rs2_source="";
        iq[i].rs2_value=-2;    
        iq[i].ins_num=-1;  
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    int line = 0;
    //while (fscanf(FP, "%lx %d %d %d %d", &pci, &op_type, &desti, &src1, &src2) != EOF)
    while(current_cycle<=11000)
    {
        current_cycle++;
        //cout<<"\n";
        for(int i=0;i<width;i++)
        {
            if ((fscanf(FP, "%lx %d %d %d %d", &pci, &op_type, &desti, &src1, &src2) != EOF))
            {
                // printf("%lx %d %d %d %d", pci, op_type, desti, src1, src2); // Print to check if inputs have been read correctly
                //cout << "\n####################Current cycle : " << current_cycle;
                ins[line].ins_nu = line;
                ins[line].fe_s = 0;
                ins[line].fe_t = 0;
                ins[line].de_s = 0;
                ins[line].de_t = 0;
                ins[line].rn_s = 0;
                ins[line].rn_t = 0;
                ins[line].rr_s = 0;
                ins[line].rr_t = 0;
                ins[line].di_s = 0;
                ins[line].di_t = 0;
                ins[line].is_s = 0;
                ins[line].is_t = 0;
                ins[line].ex_s = 0;
                ins[line].ex_t = 0;
                ins[line].wb_s = 0;
                ins[line].wb_t = 0;
                ins[line].rt_s = 0;
                ins[line].rt_t = 0;
                ins[line].pc = pci;
                ins[line].op = op_type;
                ins[line].dest = desti;
                ins[line].sc1 = src1;
                ins[line].r_sc1 = 0;
                ins[line].r_sc1_source = "";
                ins[line].r_sc1_ready = 0;
                ins[line].sc2 = src2;
                ins[line].r_sc2 = 0;
                ins[line].r_sc2_source = "";
                ins[line].r_sc2_ready = 0;
                ins[line].ready_for_fe = 1;
                ins[line].ready_for_de = 0;
                ins[line].ready_for_rn = 0;
                ins[line].ready_for_rr = 0;
                ins[line].ready_for_di = 0;
                ins[line].ready_for_is = 0;
                ins[line].ready_for_ex = 0;
                ins[line].placed_in_iq = 0;
                ins[line].renamed_once = 0;
                ins[line].rw1_ready = 0;
                ins[line].rw2_ready = 0;
                if (ins[line].op == 0)
                {
                    ins[line].ex_cycle = 1;
                }
                else if (ins[line].op == 1)
                {
                    ins[line].ex_cycle = 2;
                }
                else if (ins[line].op == 2)
                {
                    ins[line].ex_cycle = 5;
                }
                line++;
            }
        }
        Retire(ins,rob,iq,width,rob_size,iq_size);
        Writeback(ins,rob,width);
        Execute(ins,iq,dq,width,iq_size,rn);
        Issue(ins,iq,rob,width,iq_size);
        Dispatch(ins,dq,rob,iq,width,iq_size);
        Reg_Read(ins,rob,rr,width);
        Rename(ins,rob,width,rob_size,rn);
        Decode(ins,width);
        Fetch(ins,width);
        //line++;
    }

    //cout<<"\n\n\n\n====================================Final Contents============================================";
    for(int k=0;k<=9999;k++)
    {
        cout<<ins[k].ins_nu<<" "<<"fu{"<<ins[k].op<<"} src{"<<ins[k].sc1<<","<<ins[k].sc2<<"} dst{"<<ins[k].dest<<"} FE{"<<ins[k].fe_s<<","<<ins[k].fe_t<<"} DE{"<<ins[k].de_s<<","<<ins[k].de_t<<"} RN{"<<ins[k].rn_s<<","<<ins[k].rn_t<<"} RR{"<<ins[k].rr_s<<","<<ins[k].rr_t<<"} DI{"<<ins[k].di_s<<","<<ins[k].di_t<<"} IS{"<<ins[k].is_s<<","<<ins[k].is_t<<"} EX{"<<ins[k].ex_s<<","<<ins[k].ex_t<<"} WB{"<<ins[k].wb_s<<","<<ins[k].wb_t<<"} RT{"<<ins[k].rt_s<<","<<ins[k].rt_t<<"}";
        cout<<"\n";
    }
    float a=ins[9999].rt_s + ins[9999].rt_t;
    float b=10000;
    float c=b/a;
    float p;
    int add_zero=0;
    float value = (int)(c*100 + 0.5);
    p= (float)value /100;
    int p1=p*100;
    if((p1%100)==0)
    {
        add_zero=2;
    }
   
    else if((p1%10)==0)
    {
        add_zero=1;
    }

    cout<<"# === Simulator Command =========";
    cout<<"\n# ./sim "<<rob_size<<" "<<iq_size<<" "<<width<<" "<<trace_file;
    cout<<"\n# === Processor Configuration ===";
    cout<<"\n# ROB_SIZE = "<<rob_size;
    cout<<"\n# IQ_SIZE  = "<<iq_size;
    cout<<"\n# WIDTH    = "<<width;
    cout<<"\n# === Simulation Results ========";
    cout<<"\n# Dynamic Instruction Count    = 10000";
    cout<<"\n# Cycles                       = "<<ins[9999].rt_s + ins[9999].rt_t;
    if(add_zero==1)
    {
        cout<<"\n# Instructions Per Cycle (IPC) = "<<p<<"0";
    }
    else if(add_zero==2)
    {
        cout<<"\n# Instructions Per Cycle (IPC) = "<<p<<".00";
    }
    else
    {
        cout<<"\n# Instructions Per Cycle (IPC) = "<<p;
    }

    return 0;
}