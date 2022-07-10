#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
//#include <windows.h>
#include <pmmintrin.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

const int NUM_THREADS=7;
pthread_barrier_t barrier_elimination;
pthread_barrier_t barrier_update;
int count_eliminated=0;
int maxColNum=0;
int elitNum=0; //消元子
int elitLineNum=0; //被消元行
__m128i t1,t2,t3,t4;
typedef struct {
    int t_id;
}threadParam_t;
//创建位图来将倒排链表数据存入位图中
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
        int j;
        for(j=0;j<byteNum-4;j+=4)
        {
            t1=_mm_loadu_si128((__m128i*)(this->byteArray+j));
            t2=_mm_loadu_si128((__m128i*)(b.byteArray+j));
            t3=_mm_xor_si128(t1,t2);
            _mm_storeu_si128((__m128i*)(this->byteArray+j),t3);

        }
        for(;j<byteNum;j++)
        {
            this->byteArray[j]=this->byteArray[j]^b.byteArray[j];
        }

        return true;
    }
    
    /*bool xorBitList(BitList b)
    {
        if(this->bitNum!=b.bitNum)
            return false;
        for(int z=0;z<byteNum;z++)
        {
            this->byteArray[z]=this->byteArray[z]^b.byteArray[z];
        }
        return true;

    }*/
    
    //设置被消元行为消元子
    void setBitList(BitList b)
    {
        this->bitNum=b.bitNum;
        this->byteNum=b.byteNum;
        this->pivot=b.pivot;
        int j;
        for(j=0;j<byteNum-4;j+=4)
        {
            t4=_mm_loadu_si128((__m128i*)(b.byteArray+j));
            _mm_storeu_si128((__m128i*)(this->byteArray+j),t4);

        }
        for(;j<byteNum;j++)
        {
            this->byteArray[j]=b.byteArray[j];
        }
    }
    
    /*void setBitList(BitList b)
    {
        this->bitNum=b.bitNum;
        this->byteNum=b.byteNum;
        this->pivot=b.pivot;
        for(int j=0;j<byteNum;j++)
        {
            this->byteArray[j]=b.byteArray[j];
        }
    }*/
    
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

//输入最大列数、非零消元子行数、被消元行数
void inputNum()
{
    cout<<"输入最大列数："<<endl;
    cin>>maxColNum;
    cout<<"输入非零消元子的个数："<<endl;
    cin>>elitNum;
    cout<<"输入被消元行的个数："<<endl;
    cin>>elitLineNum;
}
//输入数据
void inputData()
{
    int data;
    elit=new BitList[maxColNum];//需要向其中不断添加变成消元子的被消元行
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

void *threadFunc(void* param)
{
    threadParam_t* p = (threadParam_t*)param;
    int t_id = p->t_id;
    while(count_eliminated<elitLineNum)
    {
        for(int i=t_id+count_eliminated+1;i<elitLineNum;i+=NUM_THREADS)
        {
            while(!elitLine[i].isNull())
            {
                bool flag=elit[elitLine[i].getPivot()].isNull();
                if(!flag)
                {
                    elitLine[i].xorBitList(elit[elitLine[i].getPivot()]);
                    elitLine[i].resetPivot();
                }
                else
                {
                    break;
                }
            }
        }
        pthread_barrier_wait(&barrier_elimination);
        if(t_id==0)
        {
            while(!elitLine[count_eliminated].isNull())
            {
                bool flag=elit[elitLine[count_eliminated].getPivot()].isNull();
                if(!flag)
                {
                    elitLine[count_eliminated].xorBitList(elit[elitLine[count_eliminated].getPivot()]);
                    elitLine[count_eliminated].resetPivot();
                }
                else
                {
                    elit[elitLine[count_eliminated].getPivot()].setBitList(elitLine[count_eliminated]);
                    break;
                }
            }
            ++count_eliminated;
        }
        pthread_barrier_wait(&barrier_update);
    }
}
void pthreadGauss()
{
	pthread_barrier_init(&barrier_elimination,NULL,NUM_THREADS);
	pthread_barrier_init(&barrier_update, NULL, NUM_THREADS);
    pthread_t handles[NUM_THREADS];
	threadParam_t param[NUM_THREADS];
	for(int t_id=0;t_id<NUM_THREADS;t_id++)
    {
		param[t_id].t_id=t_id;
		pthread_create(&handles[t_id],NULL,threadFunc,(void*)(param+t_id));
	}

    for(int t_id=0;t_id<NUM_THREADS;t_id++)
    {
        pthread_join(handles[t_id], NULL);
    }
    pthread_barrier_destroy(&barrier_elimination);
    pthread_barrier_destroy(&barrier_update);
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
    long long head, tail, freq;

    inputNum();
    inputData();

    struct timeval start;
    struct timeval end;
    unsigned long diff;
    gettimeofday(&start,NULL);
    pthreadGauss();
    gettimeofday(&end,NULL);
    diff=1000000*(end.tv_sec-start.tv_sec)+end.tv_usec-start.tv_usec;
    cout <<"最大列数: "<<maxColNum<< " 非零消元子: " << elitNum <<" 被消元行: "<<elitLineNum<<" time: "<<diff<< "us" << endl;

    //display(elitLine,elitLineNum);
}
