#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <windows.h>
#include <pmmintrin.h>
using namespace std;

int maxColNum=0;
int elitNum=0; //��Ԫ��
int elitLineNum=0; //����Ԫ��
__m128i t1,t2,t3,t4;

//����λͼ���������������ݴ���λͼ��
class BitList
{
public:
    int byteNum; //�ֽ���
    int bitNum; //λ��
    int *byteArray;
    int pivot; //��Ԫ
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
        bitNum=n; //λ�ĸ�����ÿ��Ԫ�ظ�����ͬ
        byteNum=(n+31)/32;
        byteArray=new int[byteNum];
        for(int i=0;i<byteNum;i++)
        {
            byteArray[i]=0b00000000000000000000000000000000;
        }

    }
    bool insert(int col) //�������ݸ�������1������
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
    //�����������
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
    //���ñ���Ԫ��Ϊ��Ԫ��
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

//�������������������Ԫ������������Ԫ����
void inputNum()
{
    cout<<"�������������"<<endl;
    cin>>maxColNum;
    cout<<"���������Ԫ�ӵĸ�����"<<endl;
    cin>>elitNum;
    cout<<"���뱻��Ԫ�еĸ�����"<<endl;
    cin>>elitLineNum;
}
//��������
void inputData()
{
    int data;
    elit=new BitList[maxColNum];//��Ҫ�����в�����ӱ����Ԫ�ӵı���Ԫ��
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
    elitInput.open("1.txt");
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
					elitRow=data; //����˳��������λ�ڶԽ���
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
    elitLineInput.open("2.txt");
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

void specialGaussSSE()
{
    for(int j=0;j<elitLineNum;j++)
    {
        while(!elitLine[j].isNull())
        {
            bool flag=elit[elitLine[j].getPivot()].isNull();
            if(!flag)
            {
                elitLine[j].xorBitList(elit[elitLine[j].getPivot()]);
                elitLine[j].resetPivot();
            }
            else
            {
                elit[elitLine[j].getPivot()].setBitList(elitLine[j]);
                break;
            }
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
    long long head, tail, freq;

    inputNum();
    inputData();

    QueryPerformanceFrequency((LARGE_INTEGER *)&freq );
    QueryPerformanceCounter((LARGE_INTEGER *)&head);
    specialGaussSSE();
    QueryPerformanceCounter((LARGE_INTEGER *)&tail );

    cout <<"�������: "<<maxColNum<< " ������Ԫ��: " << elitNum <<" ����Ԫ��: "<<elitLineNum<<" time: "<<(tail-head)*1000.0/freq<< "ms" << endl;

    //display(elitLine,elitLineNum);
}
