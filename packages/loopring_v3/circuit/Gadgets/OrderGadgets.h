// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Loopring Technology Limited.
// Modified by DeGate DAO, 2022
#ifndef _ORDERGADGETS_H_
#define _ORDERGADGETS_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"
#include "StorageGadgets.h"
#include "AccountGadgets.h"

#include "ethsnarks.hpp"
#include "gadgets/poseidon.hpp"
#include "utils.hpp"

using namespace ethsnarks;

namespace Loopring
{

class OrderGadget : public GadgetT
{
  public:
    VariableT blockExchangeForHash;
    // Inputs
    DualVariableGadget storageID;
    DualVariableGadget accountID;
    DualVariableGadget tokenS;
    DualVariableGadget tokenB;
    DualVariableGadget amountS;
    DualVariableGadget amountB;
    DualVariableGadget feeTokenID;
    DualVariableGadget fee;
    DualVariableGadget maxFee;
    DualVariableGadget useAppKey;
    ToBitsGadget disableAppKey;

    DualVariableGadget validUntil;
    DualVariableGadget fillAmountBorS;

    FloatGadget fFee;
    RequireAccuracyGadget requireAccuracyFee;

    VariableT taker;

    DualVariableGadget feeBips;

    // Checks
    RequireLeqGadget feeBips_leq_maxFeeBips;
    RequireLeqGadget fee_leq_maxFee;
    IfThenRequireNotEqualGadget tokenS_neq_tokenB;
    IfThenRequireNotEqualGadget amountS_notZero;
    IfThenRequireNotEqualGadget amountB_notZero;
    
    // AutoMarket: type == 6 or 7, normal order type == 0
    DualVariableGadget type;
    DualVariableGadget level;
    DualVariableGadget gridOffset;
    DualVariableGadget orderOffset;
    DualVariableGadget maxLevel;

    IfThenRequireEqualGadget ifUseAppKey_then_require_enable_switch;

    OrderGadget( //
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &blockExchange,
      const VariableT &maxFeeBips,
      const VariableT &isNormalOrder,
      const VariableT &_disableAppKey,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          blockExchangeForHash(blockExchange),
          // Inputs
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          tokenS(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenS")),
          tokenB(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenB")),
          amountS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountS")),
          amountB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountB")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          fee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fee")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),
          disableAppKey(pb, _disableAppKey, 1, FMT(prefix, ".disableAppKey")),
          fillAmountBorS(pb, 1, FMT(prefix, ".fillAmountBorS")),
          taker(make_variable(pb, FMT(prefix, ".taker"))),

          feeBips(pb, NUM_BITS_BIPS, FMT(prefix, ".feeBips")),

          type(pb, NUM_BITS_TYPE, FMT(prefix, ".type")),
          level(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".level")),
          gridOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".gridOffset")),
          orderOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".orderOffset")),
          maxLevel(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".maxLevel")),

          // Checks
          feeBips_leq_maxFeeBips(
            pb,
            feeBips.packed,
            maxFeeBips,
            NUM_BITS_BIPS,
            FMT(prefix, ".feeBips <= maxFeeBips")),
          tokenS_neq_tokenB(pb, isNormalOrder, tokenS.packed, tokenB.packed, FMT(prefix, ".tokenS != tokenB")),
          amountS_notZero(pb, isNormalOrder, amountS.packed, constants._0, FMT(prefix, ".amountS != 0")),
          amountB_notZero(pb, isNormalOrder, amountB.packed, constants._0, FMT(prefix, ".amountB != 0")),

          // require fee <= maxFee
          fee_leq_maxFee(pb, fee.packed, maxFee.packed, NUM_BITS_AMOUNT, FMT(prefix, ".fee <= maxFee")),
          fFee(pb, constants, Float16Encoding, FMT(prefix, ".fFee")),
          requireAccuracyFee(
            pb,
            fFee.value(),
            fee.packed,
            Float16Accuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyFee")),

          ifUseAppKey_then_require_enable_switch(
            pb,
            useAppKey.packed,
            disableAppKey.packed,
            constants._0,
            FMT(prefix, ".ifUseAppKey_then_require_enable_switch"))
          
    {
    }

    void generate_r1cs_witness(const Order &order)
    {
        LOG(LogDebug, "in OrderGadgets", "generate_r1cs_witness");
        // Inputs
        storageID.generate_r1cs_witness(pb, order.storageID);
        accountID.generate_r1cs_witness(pb, order.accountID);
        tokenS.generate_r1cs_witness(pb, order.tokenS);
        tokenB.generate_r1cs_witness(pb, order.tokenB);
        amountS.generate_r1cs_witness(pb, order.amountS);
        amountB.generate_r1cs_witness(pb, order.amountB);
        validUntil.generate_r1cs_witness(pb, order.validUntil);
        fillAmountBorS.generate_r1cs_witness(pb, order.fillAmountBorS);
        pb.val(taker) = order.taker;

        feeBips.generate_r1cs_witness(pb, order.feeBips);

        // Checks
        feeBips_leq_maxFeeBips.generate_r1cs_witness();
        tokenS_neq_tokenB.generate_r1cs_witness();
        amountS_notZero.generate_r1cs_witness();
        amountB_notZero.generate_r1cs_witness();

        // split trading fee and gas fee
        feeTokenID.generate_r1cs_witness(pb, order.feeTokenID);
        fee.generate_r1cs_witness(pb, order.fee);
        maxFee.generate_r1cs_witness(pb, order.maxFee);
        fee_leq_maxFee.generate_r1cs_witness();
        fFee.generate_r1cs_witness(toFloat(order.fee, Float16Encoding));
        requireAccuracyFee.generate_r1cs_witness();

        useAppKey.generate_r1cs_witness(pb, order.useAppKey);
        disableAppKey.generate_r1cs_witness();

        type.generate_r1cs_witness(pb, order.type);
        level.generate_r1cs_witness(pb, order.level);
        gridOffset.generate_r1cs_witness(pb, order.gridOffset);
        orderOffset.generate_r1cs_witness(pb, order.orderOffset);
        maxLevel.generate_r1cs_witness(pb, order.maxLevel);

        ifUseAppKey_then_require_enable_switch.generate_r1cs_witness();

    }

    void generate_r1cs_constraints()
    {
        LOG(LogDebug, "in OrderGadgets", "generate_r1cs_constraints");
        // Inputs
        storageID.generate_r1cs_constraints(true);
        accountID.generate_r1cs_constraints(true);
        tokenS.generate_r1cs_constraints(true);
        tokenB.generate_r1cs_constraints(true);
        amountS.generate_r1cs_constraints(true);
        amountB.generate_r1cs_constraints(true);
        validUntil.generate_r1cs_constraints(true);
        fillAmountBorS.generate_r1cs_constraints(true);

        feeBips.generate_r1cs_constraints(true);

        // Checks
        feeBips_leq_maxFeeBips.generate_r1cs_constraints();
        tokenS_neq_tokenB.generate_r1cs_constraints();
        amountS_notZero.generate_r1cs_constraints();
        amountB_notZero.generate_r1cs_constraints();

        feeTokenID.generate_r1cs_constraints(true);
        fee.generate_r1cs_constraints(true);
        maxFee.generate_r1cs_constraints(true);
        fee_leq_maxFee.generate_r1cs_constraints();
        fFee.generate_r1cs_constraints();
        requireAccuracyFee.generate_r1cs_constraints();

        useAppKey.generate_r1cs_constraints(true);
        disableAppKey.generate_r1cs_constraints();

        type.generate_r1cs_constraints(true);
        level.generate_r1cs_constraints(true);
        gridOffset.generate_r1cs_constraints(true);
        orderOffset.generate_r1cs_constraints(true);
        maxLevel.generate_r1cs_constraints(true);

        ifUseAppKey_then_require_enable_switch.generate_r1cs_constraints();

    }
};

/** 
 For grid order, we use the value of forward attribute to decide if the current order is a reverse order
 PreOrderCompleteCheckGadget is necessary when the value of attribute forward in storagte is different from the value in order. 
 */
class PreOrderCompleteCheckGadget : public GadgetT 
{
  public:
    NotEqualGadget forwardStatusNotMatch;
    TernaryGadget isNewOrder;
    TernaryGadget firstOrderFillAmount;

    IfThenRequireAccuracyGadget requireAccuracyAmount;

    PreOrderCompleteCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const VariableT &firstOrderAmountB,
      const VariableT &firstOrderAmountS,
      const VariableT &startOrderFillAmountBorS,
      const StorageReaderGadget &storage,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          forwardStatusNotMatch(pb, storage.getForward(), isForward, FMT(prefix, ".isNewOrder")),
          isNewOrder(pb, isAutoMarketOrder, forwardStatusNotMatch.result(), constants._0, FMT(prefix, ".isNewOrder")),

          firstOrderFillAmount(pb, startOrderFillAmountBorS, firstOrderAmountB, firstOrderAmountS, FMT(prefix, ".firstOrderFillAmount")),

          requireAccuracyAmount(
            pb,
            constants,
            isNewOrder.result(),
            storage.getData(),
            firstOrderFillAmount.result(),
            AutoMarketCompleteAccuracy,
            NUM_BITS_AMOUNT,
            FMT(prefix, ".requireAccuracyAmount")
          )
    {
      
    }

    void generate_r1cs_witness() 
    {
      forwardStatusNotMatch.generate_r1cs_witness();
      isNewOrder.generate_r1cs_witness();
      firstOrderFillAmount.generate_r1cs_witness();
      requireAccuracyAmount.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      forwardStatusNotMatch.generate_r1cs_constraints();
      isNewOrder.generate_r1cs_constraints();
      firstOrderFillAmount.generate_r1cs_constraints();
      requireAccuracyAmount.generate_r1cs_constraints();
    }

    const VariableT &getIsNewOrder() const
    {
        return isNewOrder.result();
    }
};

// Calculate the current latest forward value. If it is a new order, update the stored forward field. 
// If it is not a new order, return the original value directly
class NextForwardGadget : public GadgetT 
{
  public:
    NotGadget notForward;
    TernaryGadget nextForward;
    NextForwardGadget(
      ProtoboardT &pb,
      const VariableT &forward,
      const VariableT &isNewOrder,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          notForward(pb, forward, FMT(prefix, ".notForward")),
          nextForward(pb, isNewOrder, notForward.result(), forward, FMT(prefix, ".nextForward"))
    {

    }

    void generate_r1cs_witness() 
    {
      notForward.generate_r1cs_witness();
      nextForward.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      notForward.generate_r1cs_constraints();
      nextForward.generate_r1cs_constraints();
    }

    const VariableT &result() const
    {
        return nextForward.result();
    }
};

// Judge whether the current order is a forward order or a reverse order
// If it is a forward order, the amountS and amountB of the order should be consistent with the firstOrder
class ForwardOrderAmountCheckGadget : public GadgetT 
{
  public:
    BothAccuracyGadget amountSEqual;
    BothAccuracyGadget amountBEqual;
    AndGadget forwardOrderAmountOK;
    ForwardOrderAmountCheckGadget(
      ProtoboardT &pb,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        amountSEqual(pb, firstOrderAmountS, orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".amountSEqual")),
        amountBEqual(pb, firstOrderAmountB, orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".amountBEqual")),
        forwardOrderAmountOK(pb, {amountSEqual.result(), amountBEqual.result()}, FMT(prefix, ".isForwardOrder"))
    {

    }

    void generate_r1cs_witness() 
    {
      amountSEqual.generate_r1cs_witness();
      amountBEqual.generate_r1cs_witness();
      forwardOrderAmountOK.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      amountSEqual.generate_r1cs_constraints();
      amountBEqual.generate_r1cs_constraints();
      forwardOrderAmountOK.generate_r1cs_constraints();
    }

    const VariableT &forwardAmountOK() const
    {
        return forwardOrderAmountOK.result();
    }
};

class ReserveOrderFilleAmountSAmountCheckGadget : public GadgetT 
{
  public:
    BothAccuracyGadget orderAmountB_eq_firstOrderAmountS;
    IfThenSubGadget calculateReverseAmountS;
    BothAccuracyGadget orderAmountS_eq_calculateReverseAmountS;
    AndGadget amountOK;
    IfThenRequireLeqGadget requireValidOrderOffset;
    ReserveOrderFilleAmountSAmountCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const VariableT &orderOffset,
      const VariableT &isSellAndReserve,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        orderAmountB_eq_firstOrderAmountS(pb, firstOrderAmountS, orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountB_eq_firstOrderAmountS")),
        calculateReverseAmountS(pb, constants, isSellAndReserve, firstOrderAmountB, orderOffset, NUM_BITS_AMOUNT, FMT(prefix, ".calculateReverseAmountS")),
        orderAmountS_eq_calculateReverseAmountS(pb, calculateReverseAmountS.result(), orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountS_eq_calculateReverseAmountS")),
        amountOK(pb, {isSellAndReserve, orderAmountB_eq_firstOrderAmountS.result(), orderAmountS_eq_calculateReverseAmountS.result()}, FMT(prefix, ".amountOK")),
        requireValidOrderOffset(pb, constants, amountOK.result(), orderOffset, firstOrderAmountB, NUM_BITS_AMOUNT, FMT(prefix, ".requireValidOrderOffset"))
    {

    }

    void generate_r1cs_witness() 
    {
      orderAmountB_eq_firstOrderAmountS.generate_r1cs_witness();
      calculateReverseAmountS.generate_r1cs_witness();
      orderAmountS_eq_calculateReverseAmountS.generate_r1cs_witness();
      amountOK.generate_r1cs_witness();
      requireValidOrderOffset.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      orderAmountB_eq_firstOrderAmountS.generate_r1cs_constraints();
      calculateReverseAmountS.generate_r1cs_constraints();
      orderAmountS_eq_calculateReverseAmountS.generate_r1cs_constraints();
      amountOK.generate_r1cs_constraints();
      requireValidOrderOffset.generate_r1cs_constraints();
    }
    const VariableT &reserveAmountOK() const
    {
        return amountOK.result();
    }
};

class ReserveOrderFilleAmountBAmountCheckGadget : public GadgetT 
{
  public:
    BothAccuracyGadget orderAmountS_eq_firstOrderAmountB;
    // the circuit crashes if the value is out of range
    AddGadget calculateReverseAmountB;
    BothAccuracyGadget orderAmountB_eq_calculateReverseAmountB;
    AndGadget amountOK;
    ReserveOrderFilleAmountBAmountCheckGadget(
      ProtoboardT &pb,
      const VariableT &firstOrderAmountS,
      const VariableT &firstOrderAmountB,
      const OrderGadget &orderGadget,
      const VariableT &orderOffset,
      const VariableT &isBuyAndReserve,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        orderAmountS_eq_firstOrderAmountB(pb, firstOrderAmountB, orderGadget.amountS.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountS_eq_firstOrderAmountB")),
        calculateReverseAmountB(pb, firstOrderAmountS, orderOffset, NUM_BITS_AMOUNT, FMT(prefix, ".calculateReverseAmountB")),
        orderAmountB_eq_calculateReverseAmountB(pb, calculateReverseAmountB.result(), orderGadget.amountB.packed, AutoMarketAmountAccuracy, NUM_BITS_AMOUNT, FMT(prefix, ".orderAmountB_eq_calculateReverseAmountB")),
        amountOK(pb, {isBuyAndReserve, orderAmountS_eq_firstOrderAmountB.result(), orderAmountB_eq_calculateReverseAmountB.result()}, FMT(prefix, ".amountOK"))
    {

    }

    void generate_r1cs_witness() 
    {
      orderAmountS_eq_firstOrderAmountB.generate_r1cs_witness();
      calculateReverseAmountB.generate_r1cs_witness();
      orderAmountB_eq_calculateReverseAmountB.generate_r1cs_witness();
      amountOK.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      orderAmountS_eq_firstOrderAmountB.generate_r1cs_constraints();
      calculateReverseAmountB.generate_r1cs_constraints();
      orderAmountB_eq_calculateReverseAmountB.generate_r1cs_constraints();
      amountOK.generate_r1cs_constraints();
    }
    const VariableT &reserveAmountOK() const
    {
        return amountOK.result();
    }
};

// Calculate the data of the initial order
// Type = 6 (sell order): fixed amountS, modified amountB
// Type = 7 (buy order): fixed amountB, modified amountS
class FirstOrderGadget : public GadgetT 
{
  public:
    EqualGadget isFixedAmountS;
    EqualGadget isFixedAmountB;

    SafeMulGadget amountOffset;
    IfThenSubGadget firstOrderAmountS;
    IfThenAddGadget firstOrderAmountB;
    
    FirstOrderGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const OrderGadget &orderGadget,
      const VariableT &startOrderAmountS,
      const VariableT &startOrderAmountB,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        amountOffset(pb, orderGadget.gridOffset.packed, orderGadget.level.packed, NUM_BITS_AMOUNT, FMT(prefix, ".amountOffset")),
        // type = 6: fixed amountS
        // type = 7: fixed amountB
        isFixedAmountS(pb, orderGadget.type.packed, constants._6, FMT(prefix, ".isFixedAmountS")),
        isFixedAmountB(pb, orderGadget.type.packed, constants._7, FMT(prefix, ".isFixedAmountB")),
        // If the amountB is fixed, then sub
        firstOrderAmountS(pb, constants, isFixedAmountB.result(), startOrderAmountS, amountOffset.result(), NUM_BITS_AMOUNT, FMT(prefix, ".firstOrderAmountS")),
        // If the amountS is fixed, then add
        firstOrderAmountB(pb, isFixedAmountS.result(), startOrderAmountB, amountOffset.result(), NUM_BITS_AMOUNT, FMT(prefix, ".firstOrderAmountB"))
    {
      
    }

    void generate_r1cs_witness() 
    {
      amountOffset.generate_r1cs_witness();
      isFixedAmountS.generate_r1cs_witness();
      isFixedAmountB.generate_r1cs_witness();
      firstOrderAmountS.generate_r1cs_witness();
      firstOrderAmountB.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      amountOffset.generate_r1cs_constraints();
      isFixedAmountS.generate_r1cs_constraints();
      isFixedAmountB.generate_r1cs_constraints();
      firstOrderAmountS.generate_r1cs_constraints();
      firstOrderAmountB.generate_r1cs_constraints();
    }

    const VariableT &getAmountS() const
    {
        return firstOrderAmountS.result();
    }

    const VariableT &getAmountB() const
    {
        return firstOrderAmountB.result();
    }

};
class GridOrderForwardCheckGadget : public GadgetT 
{
  public:
    ForwardOrderAmountCheckGadget forwardOrderCheckGadget;

    AndGadget needCheckForwardOrderAmount;
    IfThenRequireGadget requireValidForwardAmount;

    EqualGadget isBuy;
    AndGadget isBuy_and_isForward;
    IfThenRequireEqualGadget requireBuyOrderFillAmountBorSValid;

    EqualGadget isSell;
    AndGadget isSell_and_isForward;
    IfThenRequireEqualGadget requireSellOrderFillAmountBorSValid;

    GridOrderForwardCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
      forwardOrderCheckGadget(pb, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, FMT(prefix, ".forwardOrderCheckGadget")),
      
      needCheckForwardOrderAmount(pb, {isForward, isAutoMarketOrder}, FMT(prefix, ".needCheckForwardOrderAmount")),
      requireValidForwardAmount(pb, needCheckForwardOrderAmount.result(), forwardOrderCheckGadget.forwardAmountOK(), FMT(prefix, ".requireValidForwardAmount")),

      isBuy(pb, orderType, constants._7, FMT(prefix, ".isBuy")),
      isBuy_and_isForward(pb, {isBuy.result(), isForward}, FMT(prefix, ".isBuy_and_isForward")),
      requireBuyOrderFillAmountBorSValid(pb, isBuy_and_isForward.result(), orderGadget.fillAmountBorS.packed, constants._1, FMT(prefix, ".requireBuyOrderFillAmountBorSValid")),

      isSell(pb, orderType, constants._6, FMT(prefix, ".isSell")),
      isSell_and_isForward(pb, {isSell.result(), isForward}, FMT(prefix, ".isSell_and_isForward")),
      requireSellOrderFillAmountBorSValid(pb, isSell_and_isForward.result(), orderGadget.fillAmountBorS.packed, constants._0, FMT(prefix, ".requireSellOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      forwardOrderCheckGadget.generate_r1cs_witness();
      needCheckForwardOrderAmount.generate_r1cs_witness();
      requireValidForwardAmount.generate_r1cs_witness();

      isBuy.generate_r1cs_witness();
      isBuy_and_isForward.generate_r1cs_witness();
      requireBuyOrderFillAmountBorSValid.generate_r1cs_witness();

      isSell.generate_r1cs_witness();
      isSell_and_isForward.generate_r1cs_witness();
      requireSellOrderFillAmountBorSValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      forwardOrderCheckGadget.generate_r1cs_constraints();
      needCheckForwardOrderAmount.generate_r1cs_constraints();
      requireValidForwardAmount.generate_r1cs_constraints();

      isBuy.generate_r1cs_constraints();
      isBuy_and_isForward.generate_r1cs_constraints();
      requireBuyOrderFillAmountBorSValid.generate_r1cs_constraints();

      isSell.generate_r1cs_constraints();
      isSell_and_isForward.generate_r1cs_constraints();
      requireSellOrderFillAmountBorSValid.generate_r1cs_constraints();
    }

    const VariableT &getIsBuy() const
    {
        return isBuy.result();
    }

    const VariableT &getIsSell() const
    {
        return isSell.result();
    }

};

class GridOrderReserveBuyCheckGadget : public GadgetT 
{
  public:
    EqualGadget isBuy;
    AndGadget isBuy_and_isReserve;
    EqualGadget orderFillAmoutB;
    AndGadget fillAmountB_and_reserve_order;

    ReserveOrderFilleAmountBAmountCheckGadget reserveOrderFilleAmountBAmountCheck;
    AndGadget needCheckReverseFillAmountBAmount;
    IfThenRequireGadget requireValidReverseFillAmountBAmount;

    IfThenRequireEqualGadget requireOrderFillAmountBorSValid;
    GridOrderReserveBuyCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isReverse,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix), 
      isBuy(pb, orderType, constants._7, FMT(prefix, ".isBuy")),
      isBuy_and_isReserve(pb, {isBuy.result(), isReverse}, FMT(prefix, ".isBuy_and_isReserve")),
      orderFillAmoutB(pb, startOrderFillAmountBorS, constants._1, FMT(prefix, ".orderFillAmoutB")),
      fillAmountB_and_reserve_order(pb, {orderFillAmoutB.result(), isReverse}, FMT(prefix, ".fillAmountB_and_reserve_order")),

      reserveOrderFilleAmountBAmountCheck(pb, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, orderGadget.orderOffset.packed, isBuy_and_isReserve.result(), FMT(prefix, ".reserveOrderFilleAmountBAmountCheck")),
      needCheckReverseFillAmountBAmount(pb, {fillAmountB_and_reserve_order.result(), isAutoMarketOrder}, FMT(prefix, ".needCheckReverseFillAmountBAmount")),
      requireValidReverseFillAmountBAmount(pb, needCheckReverseFillAmountBAmount.result(), reserveOrderFilleAmountBAmountCheck.reserveAmountOK(), FMT(prefix, ".requireValidReverseFillAmountBAmount")),

      requireOrderFillAmountBorSValid(pb, isBuy_and_isReserve.result(), orderGadget.fillAmountBorS.packed, constants._0, FMT(prefix, ".requireOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      isBuy.generate_r1cs_witness();
      isBuy_and_isReserve.generate_r1cs_witness();

      orderFillAmoutB.generate_r1cs_witness();
      fillAmountB_and_reserve_order.generate_r1cs_witness();

      reserveOrderFilleAmountBAmountCheck.generate_r1cs_witness();
      needCheckReverseFillAmountBAmount.generate_r1cs_witness();
      requireValidReverseFillAmountBAmount.generate_r1cs_witness();

      requireOrderFillAmountBorSValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      isBuy.generate_r1cs_constraints();
      isBuy_and_isReserve.generate_r1cs_constraints();

      orderFillAmoutB.generate_r1cs_constraints();
      fillAmountB_and_reserve_order.generate_r1cs_constraints();

      reserveOrderFilleAmountBAmountCheck.generate_r1cs_constraints();
      needCheckReverseFillAmountBAmount.generate_r1cs_constraints();
      requireValidReverseFillAmountBAmount.generate_r1cs_constraints();

      requireOrderFillAmountBorSValid.generate_r1cs_constraints();
    }

};
class GridOrderReserveSellCheckGadget : public GadgetT 
{
  public:
    EqualGadget isSell;
    AndGadget isSell_and_isReserve;

    EqualGadget orderFillAmoutS;
    AndGadget fillAmountS_and_reserve_order;

    ReserveOrderFilleAmountSAmountCheckGadget reserveOrderFilleAmountSAmountCheck;
    AndGadget needCheckReverseFillAmountSAmount;
    IfThenRequireGadget requireValidReverseFillAmountSAmount;

    IfThenRequireEqualGadget requireOrderFillAmountBorSValid;
    GridOrderReserveSellCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isAutoMarketOrder,
      const VariableT &isReverse,
      const OrderGadget &orderGadget,
      const FirstOrderGadget &firstOrderGadget,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const std::string &prefix
    ) : GadgetT(pb, prefix), 
      isSell(pb, orderType, constants._6, FMT(prefix, ".isSell")),
      isSell_and_isReserve(pb, {isSell.result(), isReverse}, FMT(prefix, ".isSell_and_isReserve")),
      orderFillAmoutS(pb, startOrderFillAmountBorS, constants._0, FMT(prefix, ".orderFillAmoutS")),
      fillAmountS_and_reserve_order(pb, {orderFillAmoutS.result(), isReverse}, FMT(prefix, ".fillAmountS_and_reserve_order")),

      reserveOrderFilleAmountSAmountCheck(pb, constants, firstOrderGadget.getAmountS(), firstOrderGadget.getAmountB(), orderGadget, orderGadget.orderOffset.packed, isSell_and_isReserve.result(), FMT(prefix, ".reserveOrderFilleAmountSAmountCheck")),
      needCheckReverseFillAmountSAmount(pb, {fillAmountS_and_reserve_order.result(), isAutoMarketOrder}, FMT(prefix, ".needCheckReverseFillAmountSAmount")),
      requireValidReverseFillAmountSAmount(pb, needCheckReverseFillAmountSAmount.result(), reserveOrderFilleAmountSAmountCheck.reserveAmountOK(), FMT(prefix, ".requireValidReverseFillAmountSAmount")),

      requireOrderFillAmountBorSValid(pb, isSell_and_isReserve.result(), orderGadget.fillAmountBorS.packed, constants._1, FMT(prefix, ".requireOrderFillAmountBorSValid"))
    {

    }

    void generate_r1cs_witness() 
    {
      isSell.generate_r1cs_witness();
      isSell_and_isReserve.generate_r1cs_witness();
      orderFillAmoutS.generate_r1cs_witness();
      fillAmountS_and_reserve_order.generate_r1cs_witness();

      reserveOrderFilleAmountSAmountCheck.generate_r1cs_witness();
      needCheckReverseFillAmountSAmount.generate_r1cs_witness();
      requireValidReverseFillAmountSAmount.generate_r1cs_witness();

      requireOrderFillAmountBorSValid.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      isSell.generate_r1cs_constraints();
      isSell_and_isReserve.generate_r1cs_constraints();

      orderFillAmoutS.generate_r1cs_constraints();
      fillAmountS_and_reserve_order.generate_r1cs_constraints();

      reserveOrderFilleAmountSAmountCheck.generate_r1cs_constraints();
      needCheckReverseFillAmountSAmount.generate_r1cs_constraints();
      requireValidReverseFillAmountSAmount.generate_r1cs_constraints();

      requireOrderFillAmountBorSValid.generate_r1cs_constraints();
    }

};

class GridOrderCheckGadget : public GadgetT 
{
  public:
    FirstOrderGadget firstOrderGadget;

    // If it is a forward order, verify whether the data of the forward order is correct
    GridOrderForwardCheckGadget forwardCheck;
    // If the reverse order is buy order, type = 7, check whether the reverse order data is correct
    GridOrderReserveBuyCheckGadget reserveBuyCheck;
    // If the reverse order is sell order, type = 6, check whether the reverse order data is correct
    GridOrderReserveSellCheckGadget reserveSellCheck;
    GridOrderCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &startOrderAmountS,
      const VariableT &startOrderAmountB,
      const VariableT &isAutoMarketOrder,
      const VariableT &isForward,
      const VariableT &isReverse,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &orderType,
      const OrderGadget &orderGadget,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
        firstOrderGadget(pb, constants, orderGadget, startOrderAmountS, startOrderAmountB, FMT(prefix, ".firstOrderGadget")),

        forwardCheck(pb, constants, isAutoMarketOrder, isForward, orderGadget, firstOrderGadget, orderType, FMT(prefix, ".forwardCheck")),

        reserveBuyCheck(pb, constants, isAutoMarketOrder, isReverse, orderGadget, firstOrderGadget, startOrderFillAmountBorS, orderType, FMT(prefix, ".reserveBuyCheck")),
        
        reserveSellCheck(pb, constants, isAutoMarketOrder, isReverse, orderGadget, firstOrderGadget, startOrderFillAmountBorS, orderType, FMT(prefix, ".reserveSellCheck"))
    {
      
    }
    void generate_r1cs_witness() 
    {
      LOG(LogDebug, "in GridOrderCheckGadget", "generate_r1cs_witness");
      firstOrderGadget.generate_r1cs_witness();
      forwardCheck.generate_r1cs_witness();
      reserveBuyCheck.generate_r1cs_witness();
      reserveSellCheck.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {
      firstOrderGadget.generate_r1cs_constraints();
      forwardCheck.generate_r1cs_constraints();
      reserveBuyCheck.generate_r1cs_constraints();
      reserveSellCheck.generate_r1cs_constraints();
    }

    const VariableT &getFirstOrderAmountS() const
    {
        return firstOrderGadget.getAmountS();
    }

    const VariableT &getFirstOrderAmountB() const
    {
        return firstOrderGadget.getAmountB();
    }

    const VariableT &getIsBuy() const
    {
        return forwardCheck.getIsBuy();
    }

    const VariableT &getIsSell() const
    {
        return forwardCheck.getIsSell();
    }

};

// if type == 6, then startOrder.fillAmountBorS = 1
// if type == 7, then startOrder.fillAmountBorS = 0
class FillAmountBorSCheckGadget  : public GadgetT 
{
  public:
    EqualGadget fillAmountB;
    EqualGadget fillAmountS;
    AndGadget validBuy;
    AndGadget validSell;

    OrGadget validFillAmountBorS;
    IfThenRequireGadget requireValidFillAmountBorS;
    FillAmountBorSCheckGadget(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &isBuy,
      const VariableT &isSell,
      const VariableT &startOrderFillAmountBorS,
      const VariableT &isAutoMarketOrder,
      const std::string &prefix
    ) : GadgetT(pb, prefix),
      fillAmountB(pb, startOrderFillAmountBorS, constants._1, FMT(prefix, ".fillAmountB")),
      fillAmountS(pb, startOrderFillAmountBorS, constants._0, FMT(prefix, ".fillAmountS")),

      validBuy(pb, {isBuy, fillAmountB.result()}, FMT(prefix, ".validBuy")),
      validSell(pb, {isSell, fillAmountS.result()}, FMT(prefix, ".validSell")),

      validFillAmountBorS(pb, {validBuy.result(), validSell.result()}, FMT(prefix, ".validFillAmountBorS")),

      requireValidFillAmountBorS(pb, isAutoMarketOrder, validFillAmountBorS.result(), FMT(prefix, ".requireValidFillAmountBorS"))
    {

    }

    void generate_r1cs_witness() 
    {
      fillAmountB.generate_r1cs_witness();
      fillAmountS.generate_r1cs_witness();
      validBuy.generate_r1cs_witness();
      validSell.generate_r1cs_witness();
      validFillAmountBorS.generate_r1cs_witness();
      requireValidFillAmountBorS.generate_r1cs_witness();
    }

    void generate_r1cs_constraints() 
    {
      fillAmountB.generate_r1cs_constraints();
      fillAmountS.generate_r1cs_constraints();
      validBuy.generate_r1cs_constraints();
      validSell.generate_r1cs_constraints();
      validFillAmountBorS.generate_r1cs_constraints();
      requireValidFillAmountBorS.generate_r1cs_constraints();
    }
};

/**
 check gird order
 1. type must be 6 or 7
 2. type == 6, fixed amountS
 3. type == 7, fixed amountB
 3. The data of the startOrder and subsequent orders are exactly the same, contains storageID, accountID, feeTokenID, maxFee, 
     validUntil, taker, type, gridOffset, orderOffset, maxLevel, useAppKey
 4. Order price verification
 5. Order fill verification
 6. Hash verification
 7. Reset storage if necessary
 8. Check if grid order has expired
 9. Check if gird order is canceled
 10. Check if the level of the order exceeds MaxLevel
 11. verify StorageID = startOrderStorageID + n(level)
*/ 
class AutoMarketOrderCheck : public GadgetT 
{
  public:
    DualVariableGadget storageID;
    DualVariableGadget accountID;
    DualVariableGadget tokenS;
    DualVariableGadget tokenB;
    DualVariableGadget amountS;
    DualVariableGadget amountB;
    DualVariableGadget feeTokenID;
    DualVariableGadget maxFee;
    DualVariableGadget feeBips;

    DualVariableGadget validUntil;
    DualVariableGadget fillAmountBorS;
    VariableT taker;

    DualVariableGadget orderType;
    DualVariableGadget gridOffset;
    DualVariableGadget orderOffset;
    DualVariableGadget maxLevel;
    DualVariableGadget useAppKey;

    EqualGadget isAutoMarketBuyOrder;
    EqualGadget isAutoMarketSellOrder;
    OrGadget isAutoMarketOrder;

    AddGadget autoMarketStorageID;

    TernaryGadget tokenSIDForStorageUpdate;
    TernaryGadget tokenBIDForStorageUpdate;

    IfThenRequireEqualGadget storageIDEqual;
    IfThenRequireEqualGadget accountIDEqual;
    IfThenRequireEqualGadget feeTokenIDEqual;
    IfThenRequireEqualGadget maxFeeEqual;
    IfThenRequireEqualGadget validUntilEqual;
    IfThenRequireEqualGadget takerEqual;
    IfThenRequireEqualGadget orderTypeEqual;
    IfThenRequireEqualGadget gridOffsetEqual;
    IfThenRequireEqualGadget orderOffsetEqual;
    IfThenRequireEqualGadget maxLevelEqual;
    IfThenRequireEqualGadget feeBipsEqual;
    IfThenRequireEqualGadget useAppKeyEqual;

    EqualGadget tokenSSEqual;
    EqualGadget tokenBBEqual;
    EqualGadget tokenSBEqual;
    EqualGadget tokenBSEqual;

    AndGadget isForward;
    AndGadget isReverse;

    OrGadget tokenIdValid;
    IfThenRequireGadget requireTokenIDValid;

    GridOrderCheckGadget gridOrderCheck;
    PreOrderCompleteCheckGadget preOrderCompleteCheck;
    NextForwardGadget nextForward;

    FillAmountBorSCheckGadget fillAmountBorSCheck;

    RequireLeqGadget requireLevelValid;

    TernaryGadget hashStorageID;
    TernaryGadget hashTokenS;
    TernaryGadget hashTokenB;
    TernaryGadget hashAmountS;
    TernaryGadget hashAmountB;
    TernaryGadget hashFillAmountBorS;

    Poseidon_18 hash;

    AutoMarketOrderCheck(
      ProtoboardT &pb,
      const Constants &constants,
      const VariableT &timestamp,
      const VariableT &blockExchange,
      const OrderGadget &orderGadget,
      const StorageReaderGadget &storage,
      const std::string &prefix)
        : GadgetT(pb, prefix),
          storageID(pb, NUM_BITS_STORAGEID, FMT(prefix, ".storageID")),
          accountID(pb, NUM_BITS_ACCOUNT, FMT(prefix, ".accountID")),
          tokenS(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenS")),
          tokenB(pb, NUM_BITS_TOKEN, FMT(prefix, ".tokenB")),
          amountS(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountS")),
          amountB(pb, NUM_BITS_AMOUNT, FMT(prefix, ".amountB")),
          validUntil(pb, NUM_BITS_TIMESTAMP, FMT(prefix, ".validUntil")),
          feeTokenID(pb, NUM_BITS_TOKEN, FMT(prefix, ".feeTokenID")),
          maxFee(pb, NUM_BITS_AMOUNT, FMT(prefix, ".maxFee")),
          feeBips(pb, NUM_BITS_BIPS, FMT(prefix, ".feeBips")),
          fillAmountBorS(pb, 1, FMT(prefix, ".fillAmountBorS")),
          taker(make_variable(pb, FMT(prefix, ".taker"))),

          orderType(pb, NUM_BITS_TYPE, FMT(prefix, ".orderType")),
          gridOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".gridOffset")),
          orderOffset(pb, NUM_BITS_AMOUNT, FMT(prefix, ".orderOffset")),
          maxLevel(pb, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".maxLevel")),
          useAppKey(pb, NUM_BITS_BYTE, FMT(prefix, ".useAppKey")),

          isAutoMarketSellOrder(pb, orderType.packed, constants._6, FMT(prefix, ".isAutoMarketBuyOrder")),
          isAutoMarketBuyOrder(pb, orderType.packed, constants._7, FMT(prefix, ".isAutoMarketBuyOrder")),
          isAutoMarketOrder(pb, {isAutoMarketBuyOrder.result(), isAutoMarketSellOrder.result()}, FMT(prefix, ".isAutoMarketOrder")),

          autoMarketStorageID(pb, storageID.packed, orderGadget.level.packed, NUM_BITS_STORAGEID, FMT(prefix, ".autoMarketStorageID")),

          tokenSIDForStorageUpdate(pb, isAutoMarketOrder.result(), tokenS.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenSIDForStorageUpdate")),
          tokenBIDForStorageUpdate(pb, isAutoMarketOrder.result(), tokenB.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenBIDForStorageUpdate")),

          storageIDEqual(pb, isAutoMarketOrder.result(), autoMarketStorageID.result(), orderGadget.storageID.packed, FMT(prefix, ".storageIDEqual")),
          accountIDEqual(pb, isAutoMarketOrder.result(), accountID.packed, orderGadget.accountID.packed, FMT(prefix, ".accountIDEqual")),
          feeTokenIDEqual(pb, isAutoMarketOrder.result(), feeTokenID.packed, orderGadget.feeTokenID.packed, FMT(prefix, ".feeTokenIDEqual")),
          maxFeeEqual(pb, isAutoMarketOrder.result(), maxFee.packed, orderGadget.maxFee.packed, FMT(prefix, ".maxFeeEqual")),
          validUntilEqual(pb, isAutoMarketOrder.result(), validUntil.packed, orderGadget.validUntil.packed, FMT(prefix, ".validUntilEqual")),
          takerEqual(pb, isAutoMarketOrder.result(), taker, orderGadget.taker, FMT(prefix, ".takerEqual")),
          orderTypeEqual(pb, isAutoMarketOrder.result(), orderType.packed, orderGadget.type.packed, FMT(prefix, ".orderTypeEqual")),
          gridOffsetEqual(pb, isAutoMarketOrder.result(), gridOffset.packed, orderGadget.gridOffset.packed, FMT(prefix, ".gridOffsetEqual")),
          orderOffsetEqual(pb, isAutoMarketOrder.result(), orderOffset.packed, orderGadget.orderOffset.packed, FMT(prefix, ".orderOffsetEqual")),
          maxLevelEqual(pb, isAutoMarketOrder.result(), maxLevel.packed, orderGadget.maxLevel.packed, FMT(prefix, ".maxLevelEqual")),
          feeBipsEqual(pb, isAutoMarketOrder.result(), feeBips.packed, orderGadget.feeBips.packed, FMT(prefix, ".feeBipsEqual")),
          useAppKeyEqual(pb, isAutoMarketOrder.result(), useAppKey.packed, orderGadget.useAppKey.packed, FMT(prefix, ".useAppKeyEqual")),

          tokenSSEqual(pb, tokenS.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenSSEqual")),
          tokenBBEqual(pb, tokenB.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenBBEqual")),
          tokenSBEqual(pb, tokenS.packed, orderGadget.tokenB.packed, FMT(prefix, ".tokenSBEqual")),
          tokenBSEqual(pb, tokenB.packed, orderGadget.tokenS.packed, FMT(prefix, ".tokenBSEqual")),

          isForward(pb, {tokenSSEqual.result(), tokenBBEqual.result()}, FMT(prefix, ".isForward")),
          isReverse(pb, {tokenSBEqual.result(), tokenBSEqual.result()}, FMT(prefix, ".isReverse")),

          tokenIdValid(pb, {isForward.result(), isReverse.result()}, FMT(prefix, ".tokenIdValid")),
          requireTokenIDValid(pb, isAutoMarketOrder.result(), tokenIdValid.result(), FMT(prefix, ".requireTokenIDValid")),

          gridOrderCheck(pb, constants, amountS.packed, amountB.packed, isAutoMarketOrder.result(), isForward.result(), isReverse.result(), fillAmountBorS.packed, orderType.packed, orderGadget, FMT(prefix, ".gridOrderCheck")),
          
          preOrderCompleteCheck(pb, constants, isAutoMarketOrder.result(), isForward.result(), gridOrderCheck.getFirstOrderAmountB(), gridOrderCheck.getFirstOrderAmountS(), fillAmountBorS.packed, storage, FMT(prefix, ".preOrderCompleteCheck")),
          nextForward(pb, storage.getForward(), preOrderCompleteCheck.getIsNewOrder(), FMT(prefix, ".nextForward")),

          fillAmountBorSCheck(pb, constants, gridOrderCheck.getIsBuy(), gridOrderCheck.getIsSell(), fillAmountBorS.packed, isAutoMarketOrder.result(), FMT(prefix, ".fillAmountBorSCheck")),

          requireLevelValid(pb, orderGadget.level.packed, maxLevel.packed, NUM_BITS_AUTOMARKET_LEVEL, FMT(prefix, ".requireLevelValid")),

          hashStorageID(pb, isAutoMarketOrder.result(), storageID.packed, orderGadget.storageID.packed, FMT(prefix, ".hashTokenS")),
          hashTokenS(pb, isAutoMarketOrder.result(), tokenS.packed, orderGadget.tokenS.packed, FMT(prefix, ".hashTokenS")),
          hashTokenB(pb, isAutoMarketOrder.result(), tokenB.packed, orderGadget.tokenB.packed, FMT(prefix, ".hashTokenB")),
          hashAmountS(pb, isAutoMarketOrder.result(), amountS.packed, orderGadget.amountS.packed, FMT(prefix, ".hashAmountS")),
          hashAmountB(pb, isAutoMarketOrder.result(), amountB.packed, orderGadget.amountB.packed, FMT(prefix, ".hashAmountB")),
          hashFillAmountBorS(pb, isAutoMarketOrder.result(), fillAmountBorS.packed, orderGadget.fillAmountBorS.packed, FMT(prefix, ".hashFillAmountBorS")),
          hash(
            pb,
            var_array(
              {orderGadget.blockExchangeForHash,
               hashStorageID.result(),
               orderGadget.accountID.packed,
               hashTokenS.result(),
               hashTokenB.result(),
               hashAmountS.result(),
               hashAmountB.result(),
               orderGadget.validUntil.packed,
               hashFillAmountBorS.result(),
               orderGadget.taker,
               orderGadget.feeTokenID.packed,
               orderGadget.maxFee.packed,
               orderGadget.feeBips.packed,
               orderGadget.type.packed,
               orderGadget.gridOffset.packed,
               orderGadget.orderOffset.packed,
               orderGadget.maxLevel.packed,
               orderGadget.useAppKey.packed
               }),
            FMT(this->annotation_prefix, ".hash"))
    {
      
    }
    void generate_r1cs_witness(const AutoMarketOrder &autoMarketOrder) 
    {
      LOG(LogDebug, "in AutoMarketOrderCheck", "generate_r1cs_witness");

      storageID.generate_r1cs_witness(pb, autoMarketOrder.storageID);
      accountID.generate_r1cs_witness(pb, autoMarketOrder.accountID);
      tokenS.generate_r1cs_witness(pb, autoMarketOrder.tokenS);
      tokenB.generate_r1cs_witness(pb, autoMarketOrder.tokenB);
      amountS.generate_r1cs_witness(pb, autoMarketOrder.amountS);
      amountB.generate_r1cs_witness(pb, autoMarketOrder.amountB);
      validUntil.generate_r1cs_witness(pb, autoMarketOrder.validUntil);
      feeTokenID.generate_r1cs_witness(pb, autoMarketOrder.feeTokenID);
      maxFee.generate_r1cs_witness(pb, autoMarketOrder.maxFee);
      fillAmountBorS.generate_r1cs_witness(pb, autoMarketOrder.fillAmountBorS);
      pb.val(taker) = autoMarketOrder.taker;
      orderType.generate_r1cs_witness(pb, autoMarketOrder.type);
      gridOffset.generate_r1cs_witness(pb, autoMarketOrder.gridOffset);
      orderOffset.generate_r1cs_witness(pb, autoMarketOrder.orderOffset);
      maxLevel.generate_r1cs_witness(pb, autoMarketOrder.maxLevel);
      feeBips.generate_r1cs_witness(pb, autoMarketOrder.feeBips);
      useAppKey.generate_r1cs_witness(pb, autoMarketOrder.useAppKey);

      isAutoMarketBuyOrder.generate_r1cs_witness();
      isAutoMarketSellOrder.generate_r1cs_witness();
      isAutoMarketOrder.generate_r1cs_witness();

      autoMarketStorageID.generate_r1cs_witness();

      tokenSIDForStorageUpdate.generate_r1cs_witness();
      tokenBIDForStorageUpdate.generate_r1cs_witness();

      storageIDEqual.generate_r1cs_witness();
      accountIDEqual.generate_r1cs_witness();
      feeTokenIDEqual.generate_r1cs_witness();
      maxFeeEqual.generate_r1cs_witness();
      validUntilEqual.generate_r1cs_witness();
      takerEqual.generate_r1cs_witness();
      orderTypeEqual.generate_r1cs_witness();
      gridOffsetEqual.generate_r1cs_witness();
      orderOffsetEqual.generate_r1cs_witness();
      maxLevelEqual.generate_r1cs_witness();
      feeBipsEqual.generate_r1cs_witness();
      useAppKeyEqual.generate_r1cs_witness();

      tokenSSEqual.generate_r1cs_witness();
      tokenBBEqual.generate_r1cs_witness();
      tokenSBEqual.generate_r1cs_witness();
      tokenBSEqual.generate_r1cs_witness();

      isForward.generate_r1cs_witness();
      isReverse.generate_r1cs_witness();

      tokenIdValid.generate_r1cs_witness();
      requireTokenIDValid.generate_r1cs_witness();

      gridOrderCheck.generate_r1cs_witness();
      preOrderCompleteCheck.generate_r1cs_witness();
      nextForward.generate_r1cs_witness();
      fillAmountBorSCheck.generate_r1cs_witness();

      requireLevelValid.generate_r1cs_witness();

      hashStorageID.generate_r1cs_witness();
      hashTokenS.generate_r1cs_witness();
      hashTokenB.generate_r1cs_witness();
      hashAmountS.generate_r1cs_witness();
      hashAmountB.generate_r1cs_witness();
      hashFillAmountBorS.generate_r1cs_witness();
      hash.generate_r1cs_witness();
    }
    void generate_r1cs_constraints() 
    {

      storageID.generate_r1cs_constraints(true);
      accountID.generate_r1cs_constraints(true);
      tokenS.generate_r1cs_constraints(true);
      tokenB.generate_r1cs_constraints(true);
      amountS.generate_r1cs_constraints(true);
      amountB.generate_r1cs_constraints(true);
      validUntil.generate_r1cs_constraints(true);
      feeTokenID.generate_r1cs_constraints(true);
      maxFee.generate_r1cs_constraints(true);
      fillAmountBorS.generate_r1cs_constraints(true);
      orderType.generate_r1cs_constraints(true);
      gridOffset.generate_r1cs_constraints(true);
      orderOffset.generate_r1cs_constraints(true);
      maxLevel.generate_r1cs_constraints(true);
      feeBips.generate_r1cs_constraints(true);
      useAppKey.generate_r1cs_constraints(true);

      isAutoMarketBuyOrder.generate_r1cs_constraints();
      isAutoMarketSellOrder.generate_r1cs_constraints();
      isAutoMarketOrder.generate_r1cs_constraints();

      autoMarketStorageID.generate_r1cs_constraints();

      tokenSIDForStorageUpdate.generate_r1cs_constraints();
      tokenBIDForStorageUpdate.generate_r1cs_constraints();

      storageIDEqual.generate_r1cs_constraints();
      accountIDEqual.generate_r1cs_constraints();
      feeTokenIDEqual.generate_r1cs_constraints();
      maxFeeEqual.generate_r1cs_constraints();
      validUntilEqual.generate_r1cs_constraints();
      takerEqual.generate_r1cs_constraints();
      orderTypeEqual.generate_r1cs_constraints();
      gridOffsetEqual.generate_r1cs_constraints();
      orderOffsetEqual.generate_r1cs_constraints();
      maxLevelEqual.generate_r1cs_constraints();
      feeBipsEqual.generate_r1cs_constraints();
      useAppKeyEqual.generate_r1cs_constraints();

      tokenSSEqual.generate_r1cs_constraints();
      tokenBBEqual.generate_r1cs_constraints();
      tokenSBEqual.generate_r1cs_constraints();
      tokenBSEqual.generate_r1cs_constraints();

      isForward.generate_r1cs_constraints();
      isReverse.generate_r1cs_constraints();

      tokenIdValid.generate_r1cs_constraints();
      requireTokenIDValid.generate_r1cs_constraints();

      gridOrderCheck.generate_r1cs_constraints();
      preOrderCompleteCheck.generate_r1cs_constraints();
      nextForward.generate_r1cs_constraints();
      fillAmountBorSCheck.generate_r1cs_constraints();

      requireLevelValid.generate_r1cs_constraints();

      hashStorageID.generate_r1cs_constraints();
      hashTokenS.generate_r1cs_constraints();
      hashTokenB.generate_r1cs_constraints();
      hashAmountS.generate_r1cs_constraints();
      hashAmountB.generate_r1cs_constraints();
      hashFillAmountBorS.generate_r1cs_constraints();
      hash.generate_r1cs_constraints();
    }

    const VariableT &getNewForwardForStorageUpdate() const
    {
        return nextForward.result();
    }

    const VariableT &getVerifyHash() const
    {
        return hash.result();
    }

    const VariableT &isNewOrder() const
    {
        return preOrderCompleteCheck.getIsNewOrder();
    }

    const VariableT &getIsAutoMarketOrder() const
    {
        return isAutoMarketOrder.result();
    }

    const VariableT &getTokenSIDForStorageUpdate() const
    {
        return tokenSIDForStorageUpdate.result();
    }

    const VariableT &getTokenBIDForStorageUpdate() const
    {
        return tokenBIDForStorageUpdate.result();
    }
};



} // namespace Loopring



#endif
