#include "qe.h"
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>

#define DEBUG false

bool compare(int left, int right, CompOp op)
{
    switch(op)
    {
        case EQ_OP: return (left == right);
		case LT_OP: return (left < right);
		case LE_OP: return (left <= right);
		case GT_OP: return (left > right);
		case GE_OP: return (left >= right);
		case NE_OP: return (left != right);
		case NO_OP: return true;
		default: return false;
    }
}

bool compare(float left, float right, CompOp op)
{
    switch(op)
    {
        case EQ_OP: return (left == right);
		case LT_OP: return (left < right);
		case LE_OP: return (left <= right);
		case GT_OP: return (left > right);
		case GE_OP: return (left >= right);
		case NE_OP: return (left != right);
		case NO_OP: return true;
		default: return false;
    }
}

bool compare(const void* left, const void* right, CompOp op)
{
    int leftSize;
    int rightSize;
    memcpy(&leftSize, (char*) left, __SIZEOF_INT__);
    memcpy(&rightSize, (char*) right, __SIZEOF_INT__);
    char leftString[leftSize+1];
    char rightString[rightSize+1];

	memcpy (leftString, (char*)left + __SIZEOF_INT__, leftSize);
    leftString[leftSize] = '\0';

    memcpy (rightString, (char*)right + __SIZEOF_INT__, rightSize);
    rightString[rightSize] = '\0';
    int stringComp = strcmp(leftString, rightString);
    switch(op)
    {
        case EQ_OP: return (stringComp == 0);
        case LT_OP: return (stringComp < 0);
        case LE_OP: return (stringComp <= 0);
        case GT_OP: return (stringComp > 0);
        case GE_OP: return (stringComp >= 0);
        case NE_OP: return (stringComp != 0);
        case NO_OP: return true;
        default: return false;  
    }
}

RC compareValues(const void *left, const void *right,  Attribute attr, CompOp op)
{
    switch(attr.type)
    {
        case TypeInt: 
            int leftIntVal, rightIntVal;
            memcpy(&leftIntVal, left, __SIZEOF_INT__);
            memcpy(&rightIntVal, right, __SIZEOF_INT__);
            return compare(leftIntVal, rightIntVal, op);
            break;
        case TypeReal:
            float leftFloatVal, rightFloatVal;
            memcpy(&leftFloatVal, left, __SIZEOF_INT__);
            memcpy(&rightFloatVal, right, __SIZEOF_INT__);
            return compare(leftFloatVal, rightFloatVal, op);
            break;
        case TypeVarChar: return compare(left, right, op);
        default: return -1;
    }
}

Filter::Filter(Iterator *input, const Condition &condition)
{
    iter = input;
    cond = condition;
    iter->getAttributes(attrs);
}

void Filter::getAttributes (vector<Attribute> &attrs) const 
{
	iter->getAttributes(attrs);
}

unsigned findInIterator(Condition &cond, vector<Attribute> &attrs)
{
    auto predicate = [&](Attribute att) {return att.name == cond.lhsAttr;};
	auto iteratorPosition = find_if (attrs.begin(), attrs.end(), predicate);
	unsigned foundIndex = distance (attrs.begin(), iteratorPosition);
    return foundIndex;
}

RC getLeftValue(void* data, int index, int nullIndicatorSize, vector<Attribute> &attrs, void* leftVal)
{
    int size = nullIndicatorSize;
    char nullIndicator[nullIndicatorSize];
    memset(nullIndicator, 0, nullIndicatorSize);
    memcpy(nullIndicator, (char*)data, nullIndicatorSize);

    for(int i = 0; i < attrs.size(); i++)
    {
        int indexOfIndicator =i/CHAR_BIT;
        int indicatorMask  =1<<(CHAR_BIT-1-(i%CHAR_BIT));
        if((nullIndicator[indexOfIndicator] & indicatorMask) != 0)
        {
            continue;
        }
        switch(attrs[i].type)
        {
            case TypeInt:
                memcpy(leftVal, (char*)data+size, __SIZEOF_INT__);
                size+= __SIZEOF_INT__;
            break;
            case TypeReal:
                memcpy(leftVal, (char*)data+size, __SIZEOF_FLOAT__);
                size+= __SIZEOF_FLOAT__;
            break;
            case TypeVarChar:
                int attrLength = 0;
                memcpy(&attrLength, (char*)data+size, __SIZEOF_INT__);
                int totalLength = 4+attrLength;
                memcpy(leftVal, (char*)data+size, totalLength);
                size+=totalLength;
            break;
        }  
        if(i == index)
            return SUCCESS;
    }
    return FAILURE;
}

RC Filter::getNextTuple (void *data) 
{
    //int counter = 0;
    while(true)
    {
        //cout<<'\n'<<counter;
        //counter++;
        if(iter->getNextTuple(data) == QE_EOF)
        {
            if(DEBUG)
                cout<<"Returned QE_EOF in Filter::getNextTuple";
            return QE_EOF;
        }
        else if(cond.bRhsIsAttr)
        {
            if(DEBUG)
                cout<<"bRhsIsAttr in Filter::getNextTuple";
            return -1;
        }

        unsigned foundIndex = findInIterator(cond, attrs);

        if(foundIndex >= attrs.size() || cond.rhsValue.type != attrs[foundIndex].type)
        {
            if(DEBUG)
                cout<<"Filter did not find the attr";
                return 1;
        }

        int nullIndicatorSize = int(ceil((double)attrs.size()/8.0));
        char nullIndicator[nullIndicatorSize];
        memset(nullIndicator, 0, nullIndicatorSize);
		memcpy(nullIndicator, (char*)data, nullIndicatorSize);

        int indexOfIndicator = foundIndex / CHAR_BIT;
        int indicatorMask  = 1 << (CHAR_BIT - 1 - (foundIndex % CHAR_BIT));
        if((nullIndicator[indexOfIndicator] & indicatorMask) != 0)
        {
            continue;
        }
        
        void* leftVal = malloc(PAGE_SIZE);
        getLeftValue(data, foundIndex, nullIndicatorSize, attrs, leftVal);
        if(compareValues(leftVal, cond.rhsValue.data, attrs[foundIndex], cond.op))
        {
            free(leftVal);
            return SUCCESS;
        }

        free(leftVal);
        
    }
    return FAILURE;




void Project::getAttributes(vector<Attribute> &attrs) const {
	attrs.assign(this->attrs.begin(), this->attrs.end());
}

void INLJoin::getAttributes(vector<Attribute> &attrs) const{
	attrs.assign(this->attrs.begin(), this->attrs.end());
}
