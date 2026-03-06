#pragma once
#include <string>

enum class SignalId : unsigned short;
class SignalGeneric;
//Interface for receiving generic signals where explicit definitions is not convenient.
class ISignalListener
{
public:
   virtual ~ISignalListener() = default;
   virtual void ReceiveSignal(SignalId signalID) = 0;
   virtual void ReceiveSignal(SignalGeneric signalComplex) = 0;
};

enum class SignalId : unsigned short;
class SignalGeneric
{
public:
   SignalId mSignalID;

   std::string argStr1;
   std::string argStr2;
   std::string argStr3;
   std::string argStr4;

   long argLong1;
   long argLong2;
   long argLong3;
   long argLong4;

   double argDouble1;
   double argDouble2;
   double argDouble3;
   double argDouble4;
};

enum class SignalId : unsigned short
{
   Generic = 0,
   ResizeRequest = 1,
};