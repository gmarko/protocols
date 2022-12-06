import sys
import json
import copy
from collections import namedtuple
from math import *

from sparse_merkle_tree import SparseMerkleTree
from float import *

from ethsnarks.eddsa import PureEdDSA
from ethsnarks.jubjub import Point
from ethsnarks.field import FQ
from ethsnarks.merkletree import MerkleTree
from ethsnarks.poseidon import poseidon, poseidon_params
from ethsnarks.field import SNARK_SCALAR_FIELD

BINARY_TREE_DEPTH_STORAGE = 14
BINARY_TREE_DEPTH_ACCOUNTS = 32
BINARY_TREE_DEPTH_TOKENS = 32

MAX_BATCH_SPOT_TRADE_USER = 6
MAX_BATCH_SPOT_TRADE_USER_ORDER = [4,2,1,1,1,1]
# Maximum number of orders
MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX = 4
MAX_BATCH_SPOT_TRADE_ALL_USER_ORDER_MAX = 10

class Fill(object):
    def __init__(self, amountS, amountB):
        self.S = int(amountS)
        self.B = int(amountB)

class FeeInBatch(object):
    def __init__(self, first = 0, second = 0, third = 0):
        self.first = int(first)
        self.second = int(second)
        self.third = int(third)

class Calculate():
    def calculateFees(self, amountB, feeBips):
        fee = (amountB * feeBips) // 10000
        return (fee)

    # AutoMarketForward is calculated on the premise that the order must be autoMarketOrder
    # SavedForward = 1, same token with startOrder, return 1
    # SavedForward = 1, not same token with startOrder, return 0
    # SavedForward = 0, same token with startOrder, return 1
    # SavedForward = 0. not same token with startOrder, return 0
    def calculateAutoMarketForward(self, savedForward, order) :
        orderSameToken = False
        if order.tokenS == order.startOrder.tokenS and order.tokenB == order.startOrder.tokenB:
            orderSameToken = True
        if savedForward == 1 and orderSameToken:
            return 1
        if savedForward == 1 and not(orderSameToken):
            return 0
        if savedForward == 0 and orderSameToken:
            return 1
        if savedForward == 0 and not(orderSameToken):
            return 0

    def getData(self, account, storageID):
        storage = account.getStorage(int(storageID))

        # Storage trimming
        numSlots = (2 ** BINARY_TREE_DEPTH_STORAGE)
        leafStorageID = storage.storageID if int(storage.storageID) > 0 else int(storageID) % numSlots
        tokenSID = int(storage.tokenSID) if (int(storageID) == int(leafStorageID)) else 0
        tokenBID = int(storage.tokenBID) if (int(storageID) == int(leafStorageID)) else 0
        filled = int(storage.data) if (int(storageID) == int(leafStorageID)) else 0
        gasFee = int(storage.gasFee) if (int(storageID) == int(leafStorageID)) else 0
        cancelled = int(storage.cancelled) if (int(storageID) == int(leafStorageID)) else 0
        forward = int(storage.forward) if (int(storageID) == int(leafStorageID)) else 1
        return tokenSID, tokenBID, filled, gasFee, cancelled, forward
    
    def getCurrentData(self, account, storageID):
        storage = account.getStorage(int(storageID))

        return int(storage.tokenSID), int(storage.tokenBID), int(storage.data), int(storage.gasFee), int(storage.cancelled), int(storage.forward)


    def getMaxFill(self, order, filled, balanceLimit, state):
        account = state.getAccount(order.accountID)

        balanceS = int(account.getBalance(order.tokenS)) if balanceLimit else int(order.amountS)

        limit = int(order.amountB) if order.fillAmountBorS else int(order.amountS)
        filledLimited = limit if limit < filled else filled
        remaining = limit - filledLimited
        remainingS_buy = remaining * int(order.amountS) // int(order.amountB)
        remainingS = remainingS_buy if order.fillAmountBorS else remaining
        fillAmountS = balanceS if balanceS < remainingS else remainingS
        fillAmountB = fillAmountS * int(order.amountB) // int(order.amountS)
        return Fill(fillAmountS, fillAmountB)

    def match(self, takerOrder, takerFill, makerOrder, makerFill):
        if takerFill.B < makerFill.S:
            makerFill.S = takerFill.B
            makerFill.B = takerFill.B * int(makerOrder.amountB) // int(makerOrder.amountS)
        else:
            takerFill.S = makerFill.S * int(takerOrder.amountS) // int(takerOrder.amountB)
            takerFill.B = makerFill.S

        spread = takerFill.S - makerFill.B
        matchable = makerFill.B <= takerFill.S

        return (spread, matchable)
    
    def addTwoDimDict(self, thedict, key_a, key_b, val):
        if key_a in thedict:
            thedict[key_a].update({key_b: val})
        else:
            thedict.update({key_a:{key_b: val}})
    def getFromTwoDimDict(self, thedict, key_a, key_b):
        if key_a in thedict and key_b in thedict[key_a]:
            return thedict[key_a][key_b]
        else:
            return None

    def getBatchOrderBalance(self, orders, tokenID):
        deltaBalance = 0
        for order in orders:
            if order.isNoop == 1:
                continue
            if order.tokenS == tokenID:
                deltaBalance = deltaBalance - int(order.deltaFilledS)
            if order.tokenB == tokenID:
                deltaBalance = deltaBalance + int(order.deltaFilledB)
        return deltaBalance
    
    # If thirdToken is the same as firstToken or secondToken, don't need to calculate again, return 0 directly
    def getThirdBalance(self, firstTokenID, secondTokenID, thirdTokenID, orders, thirdTradingFee, thirdGasFee):
        thirdBalance = self.getBatchOrderBalance(orders, thirdTokenID) - thirdTradingFee - thirdGasFee
        if firstTokenID == thirdTokenID or secondTokenID == thirdTokenID:
            thirdBalance = 0
        return thirdBalance
    
    def getBatchSpotTradeStorageData(self, storageDataMap, accountID, storageID, fillAmountBorS, filled_B, fillB):
        filledTemp = filled_B
        if self.getFromTwoDimDict(storageDataMap, accountID, storageID) != None :
            filledTemp = self.getFromTwoDimDict(storageDataMap, accountID, storageID)
        newData = filledTemp + (fillB.B if fillAmountBorS else fillB.S)
        self.addTwoDimDict(storageDataMap, accountID, storageID, int(newData))
        return newData
    def getBatchSpotTradeStorageGasFee(self, storageGasFeeMap, accountID, storageID, gasFeeSaved, gasFeeCurrent):
        gasFeeTemp = gasFeeSaved
        if self.getFromTwoDimDict(storageGasFeeMap, accountID, storageID) != None :
            gasFeeTemp = self.getFromTwoDimDict(storageGasFeeMap, accountID, storageID)
        newGasFee = gasFeeTemp + gasFeeCurrent
        self.addTwoDimDict(storageGasFeeMap, accountID, storageID, int(newGasFee))
        return newGasFee

    def getBatchSpotTradeStorageForward(self, storageForwardMap, order, savedForward):
        forward = savedForward
        if self.getFromTwoDimDict(storageForwardMap, order.accountID, order.storageID) != None :
            forward = self.getFromTwoDimDict(storageForwardMap, order.accountID, order.storageID)
        newForward = self.calculateAutoMarketForward(forward, order)
        self.addTwoDimDict(storageForwardMap, order.accountID, order.storageID, int(newForward))
        return (newForward, forward)

    def getBatchSpotTradeOperatorThirdBalance(self, firstTokenID, secondTokenID, thirdTokenID, thirdTokenGasFeeSum, thirdTokenTradingFeeSum):
        balance = thirdTokenGasFeeSum + thirdTokenTradingFeeSum
        if firstTokenID == thirdTokenID or secondTokenID == thirdTokenID:
            balance = 0
        return balance

    def storageDefaultSet(self, accountLeaf, addressArray, tokenSIDArray, tokenBIDArray, dataArray, storageIDArray, gasFeeArray, cancelledArray, forwardArray, length):
        newAddressArray = []
        newTokenSIDArray = []
        newTokenBIDArray = []
        newForwardArray = []
        newDataArray = []
        newStorageIDArray = []
        newGasFeeArray = []
        newCancelledArray = []
        index = 0
        for address in addressArray:
            storage = accountLeaf.getStorage(address)
            newAddressArray.append(address)
            newTokenSIDArray.append(tokenSIDArray[index])
            newTokenBIDArray.append(tokenBIDArray[index])
            newDataArray.append(dataArray[index])
            newStorageIDArray.append(storageIDArray[index])
            newGasFeeArray.append(gasFeeArray[index])
            # Cancel needs to be read from the tree. If the incoming data is empty, the default value will be read. 
            # For SpotTrade, this value will not be modified, so the default value is used
            newCancelledArray.append(storage.cancelled if cancelledArray[index] is None else cancelledArray[index])
            # If it is a Auto-Market order, the forward value will certainly be updated. 
            # If it is not a Auto-Market order, the value will be constant to the default value of 1
            newForwardArray.append(forwardArray[index])
            index = index + 1
        return (newAddressArray, newTokenSIDArray, newTokenBIDArray, newDataArray, newStorageIDArray, newGasFeeArray, newCancelledArray, newForwardArray)
    
    def loopAllBacthSpotTradeOrders(self, users, tokens, state):
        tokenSIDArray = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        tokenBIDArray = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        filledArray = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        gasFeeArray = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        cancelledArray = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        forwardArray = [[1 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]

        fill = [[Fill(0, 0) for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        tradingFee = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        tradingFeeUser = [FeeInBatch(0,0,0) for i in range(MAX_BATCH_SPOT_TRADE_USER)]
        gasFeeUser = [FeeInBatch(0,0,0) for i in range(MAX_BATCH_SPOT_TRADE_USER)]
        gasFeeOrder = [[0 for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        signatures = [[{} for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        signaturesTemp = [[{} for i in range(MAX_BATCH_SPOT_TRADE_USER_ORDER_MAX)] for j in range(MAX_BATCH_SPOT_TRADE_USER)]
        # tradingFee cumulative
        firstTokenTradingFeeSum = 0
        secondTokenTradingFeeSum = 0
        thirdTokenTradingFeeSum = 0
        # gasFee cumulative
        firstTokenGasFeeSum = 0
        secondTokenGasFeeSum = 0
        thirdTokenGasFeeSum = 0

        firstTokenID = tokens[0]
        secondTokenID = tokens[1]
        thirdTokenID = tokens[2]
        
        userIndex = 0
        orderIndex = 0
        for user in users:
            if user.isNoop == 1:
                userIndex = userIndex + 1
                continue
            orderIndex = 0
            for order in user.orders:
                if order.isNoop == 1:
                    orderIndex = orderIndex + 1
                    continue
                # Fee may be beyond the range of float16, so make a choice here
                order.fee = str(roundToFloatValue(int(order.fee), Float16Encoding))

                (tokenSIDArray[userIndex][orderIndex], tokenBIDArray[userIndex][orderIndex], filledArray[userIndex][orderIndex], 
                gasFeeArray[userIndex][orderIndex], cancelledArray[userIndex][orderIndex], forwardArray[userIndex][orderIndex]) = self.getData(state.getAccount(order.accountID), order.storageID)
                # Check whether it is a grid reverse order. If it is a reverse order, the filled needs to be reset
                if order.type == 6 or order.type == 7:
                    calculateForward = self.calculateAutoMarketForward(forwardArray[userIndex][orderIndex], order)
                    if calculateForward != forwardArray[userIndex][orderIndex] :
                        filledArray[userIndex][orderIndex] = 0

                # Use real values here
                fill[userIndex][orderIndex].B = int(order.deltaFilledB)
                fill[userIndex][orderIndex].S = int(order.deltaFilledS)
                
                calculateTradingFee = self.calculateFees(fill[userIndex][orderIndex].B, order.feeBips)
                assert int(order.tradingFee) <= int(calculateTradingFee), "order.tradingFee must <= calculateTradingFee;order.tradingFee:" + str(order.tradingFee) + ";calculateTradingFee:" + str(calculateTradingFee)
                tradingFee[userIndex][orderIndex] = order.tradingFee
                
                # Calculate the tradingfee to see which token matches the order's tokenB,
                # The trading fee of three currencies is accumulated according to the user, which is used to update the user's balance
                # At the same time, all tradingfees are accumulated by currency type, which is used to update the balance of protocol
                # Because the feeTokenID is the same as a token, need to use 'if else' to prevent repeated accumulation
                if firstTokenID == order.tokenB:
                    tradingFeeUser[userIndex].first = tradingFeeUser[userIndex].first + int(tradingFee[userIndex][orderIndex])
                    firstTokenTradingFeeSum = firstTokenTradingFeeSum + int(tradingFee[userIndex][orderIndex])
                elif secondTokenID == order.tokenB:
                    tradingFeeUser[userIndex].second = tradingFeeUser[userIndex].second + int(tradingFee[userIndex][orderIndex])
                    secondTokenTradingFeeSum = secondTokenTradingFeeSum + int(tradingFee[userIndex][orderIndex])
                elif thirdTokenID == order.tokenB:
                    tradingFeeUser[userIndex].third = tradingFeeUser[userIndex].third + int(tradingFee[userIndex][orderIndex])
                    thirdTokenTradingFeeSum = thirdTokenTradingFeeSum + int(tradingFee[userIndex][orderIndex])

                # To calculate gasFee, to support the matching of three currencies, the fee here needs to distinguish tokenID, because the fee may be any currency
                # At the same time, the gasFee sum of the following three tokens of all users is accumulated
                if firstTokenID == order.feeTokenID:
                    firstTokenGasFeeSum = firstTokenGasFeeSum + int(order.fee)
                    gasFeeUser[userIndex].first = gasFeeUser[userIndex].first + int(order.fee)
                elif secondTokenID == order.feeTokenID:
                    secondTokenGasFeeSum = secondTokenGasFeeSum + int(order.fee)
                    gasFeeUser[userIndex].second = gasFeeUser[userIndex].second + int(order.fee)
                elif thirdTokenID == order.feeTokenID:
                    thirdTokenGasFeeSum = thirdTokenGasFeeSum + int(order.fee)
                    gasFeeUser[userIndex].third = gasFeeUser[userIndex].third + int(order.fee)
                # Record gasFee for storage update
                gasFeeOrder[userIndex][orderIndex] = int(order.fee)
                # Store signature information for all orders
                if orderIndex >0 and (userIndex == 0 or userIndex == 1):
                    signatures[userIndex][orderIndex - 1] = order.signature
                else:
                    signatures[userIndex][orderIndex] = order.signature
                signaturesTemp[userIndex][orderIndex] = order.signature
                orderIndex = orderIndex + 1
            userIndex = userIndex + 1
        return (tokenSIDArray, tokenBIDArray, filledArray, gasFeeArray, cancelledArray, forwardArray, 
            fill, tradingFee, tradingFeeUser, gasFeeOrder, gasFeeUser, signaturesTemp, signatures, 
            firstTokenTradingFeeSum, secondTokenTradingFeeSum, thirdTokenTradingFeeSum, 
            firstTokenGasFeeSum, secondTokenGasFeeSum, thirdTokenGasFeeSum)