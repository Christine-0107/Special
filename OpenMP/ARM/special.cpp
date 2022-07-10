#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <sys/time.h>
#include <omp.h>
using namespace std;
int maxColNum=0;
int elitNum=0; 
int elitLineNum=0; 

const int NUM_THREADS=4;
int count_elitLine=0;

class BitList
{
public:
    int byteNum; //字节数
    int bitNum; //位数
    int *byteArray;
    int pivot; //首元
public:
    BitList()
    {
        byteNum=0;
        bitNum=0;
        pivot=-1;
        byteArray=NULL;
    }
    bool isNull()
    {
        if(this->pivot==-1)
            return true;
        else
            return false;
    }
    void init(int n)
    {
        bitNum=n; //位的个数与每行元素个数相同
        byteNum=(n+31)/32;
        byteArray=new int[byteNum];
        for(int i=0;i<byteNum;i++)
        {
            byteArray[i]=0b00000000000000000000000000000000;
        }

    }
    bool insert(int col) //插入数据给出的是1所在列
    {
        if(col>=0 && col/32<byteNum)
        {
            int bits=col&31;
            byteArray[col/32]=(byteArray[col/32]) | (1<<bits);
            if(col>pivot)
                pivot=col;
            return true;
        }
        return false;

    }
    int getPivot()
    {
        return this->pivot;
    }
    void resetPivot()
    {
        int p=-1;
        for(int i=0;i<bitNum;i++)
        {
            if((byteArray[i/32]&(1<<(i&31))))
            {
                p=i;
            }
        }
        this->pivot=p;
    }
    //进行异或运算
    bool xorBitList(BitList b)
    {
        if(this->bitNum!=b.bitNum)
            return false;
        for(int i=0;i<byteNum;i++)
        {
            this->byteArray[i]=this->byteArray[i]^b.byteArray[i];
        }
        return true;
    }
    //设置被消元行为消元子
    void setBitList(BitList b)
    {
        this->bitNum=b.bitNum;
        this->byteNum=b.byteNum;
        this->pivot=b.pivot;
        for(int i=0;i<byteNum;i++)
        {
            this->byteArray[i]=b.byteArray[i];
        }
    }

    void show()
    {
        for(int i=bitNum-1;i>=0;i--)
        {
            if((byteArray[i/32])&(1<<(i&31)))
            {
                cout<<i<<" ";
            }
        }
        cout<<endl;
    }

};
BitList* elit;
BitList* elitLine;

void inputNum()
{
    cout<<"maxCol:"<<endl;
    cin>>maxColNum;
    cout<<"elitNum:"<<endl;
    cin>>elitNum;
    cout<<"elitLineNum:"<<endl;
    cin>>elitLineNum;
}

void inputData()
{
    int data;
    elit=new BitList[maxColNum];
    elitLine=new BitList[elitLineNum];
    for(int i=0;i<maxColNum;i++)
    {
        elit[i].init(maxColNum);
    }
    for(int i=0;i<elitLineNum;i++)
    {
        elitLine[i].init(maxColNum);
    }
    ifstream elitInput;
    string elitString;
    elitInput.open("7.1.txt");
    int flag1=0;
    int elitRow=0;
    while(getline(elitInput,elitString))
    {
        istringstream in(elitString);
        while(in>>data)
        {
            if(data>=0&&data<=maxColNum)
            {
                if(flag1==0)
                {
                    elitRow=data; //调整顺序让首项位于对角线
                }
                elit[elitRow].insert(data);
                flag1=1;
            }
        }
        flag1=0;
    }
    elitInput.close();
    ifstream elitLineInput;
    string elitLineString;
    elitLineInput.open("7.2.txt");
    int count=0;
    while(getline(elitLineInput,elitLineString))
    {
        istringstream in(elitLineString);
        while(in>>data)
        {
            if(data>=0&&data<=maxColNum)
            {
                elitLine[count].insert(data);
            }
        }
        count++;
    }
    elitLineInput.close();

}

void serialGauss()
{
    while(count_elitLine<elitLineNum)
    {
        for(int i=count_elitLine;i<elitLineNum;i++)
        {
            while(!elitLine[i].isNull())
            {
                bool flag=elit[elitLine[i].getPivot()].isNull();
                if(!flag) //非空，异或，重置首元
                {
                    elitLine[i].xorBitList(elit[elitLine[i].getPivot()]);
                    elitLine[i].resetPivot();
                }
                else //要升格就break
                {
                    break;
                }
            }
        }

        while(!elitLine[count_elitLine].isNull())
        {
            bool flag=elit[elitLine[count_elitLine].getPivot()].isNull();
            if(!flag)
            {
                elitLine[count_elitLine].xorBitList(elit[elitLine[count_elitLine].getPivot()]);
                elitLine[count_elitLine].resetPivot();
            }
            else
            {
                elit[elitLine[count_elitLine].getPivot()].setBitList(elitLine[count_elitLine]);
                break;
            }
        }
        ++count_elitLine;
        
    }
}

void omp_Gauss()
{
    #pragma omp parallel num_threads(NUM_THREADS)
    while(count_elitLine<elitLineNum)
    {
        //#pragma omp for schedule(dynamic,NUM_THREADS)
        #pragma omp for
        for(int i=count_elitLine;i<elitLineNum;i++)
        {
            while(!elitLine[i].isNull())
            {
                bool flag=elit[elitLine[i].getPivot()].isNull();
                if(!flag) //非空，异或，重置首元
                {
                    elitLine[i].xorBitList(elit[elitLine[i].getPivot()]);
                    elitLine[i].resetPivot();
                }
                else //要升格就break
                {
                    break;
                }
            }
        }

        #pragma omp single
        {
            while(!elitLine[count_elitLine].isNull())
            {
                bool flag=elit[elitLine[count_elitLine].getPivot()].isNull();
                if(!flag)
                {
                    elitLine[count_elitLine].xorBitList(elit[elitLine[count_elitLine].getPivot()]);
                    elitLine[count_elitLine].resetPivot();
                }
                else
                {
                    elit[elitLine[count_elitLine].getPivot()].setBitList(elitLine[count_elitLine]);
                    break;
                }
            }
            ++count_elitLine;
        }
    }
}

void display(BitList* b,int n)
{
    cout<<"Result:"<<endl;
    if((n<0)|(n>maxColNum)|(b==NULL))
        return;
    else
    {
        for(int i=0;i<n;i++)
            b[i].show();
    }
}

int main()
{
    struct timeval start;
    struct timeval end;
    unsigned long diff;
    
    inputNum();
    inputData();
    gettimeofday(&start,NULL);
    //serialGauss();
    omp_Gauss();
    gettimeofday(&end,NULL);
    diff=1000000*(end.tv_sec-start.tv_sec)+end.tv_usec-start.tv_usec;
    //cout <<"串行 "<< "最大列数: "<<maxColNum<< " 非零消元子: " << elitNum <<" 被消元行: "<<elitLineNum<<" time: "<<diff<< "us" << endl;
    cout <<"omp "<< "最大列数: "<<maxColNum<< " 非零消元子: " << elitNum <<" 被消元行: "<<elitLineNum<<" time: "<<diff<< "us" << endl;

    //display(elitLine,elitLineNum);
}
