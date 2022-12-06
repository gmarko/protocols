import BN = require("bn.js");
import { Constants, Signature } from "loopringV3.js";
import { expectThrow } from "./expectThrow";
import { ExchangeTestUtil } from "./testExchangeUtil";
import { AuthMethod, OrderInfo, SpotTrade } from "./types";

contract("Exchange", (accounts: string[]) => {
  let exchangeTestUtil: ExchangeTestUtil;

  let exchangeID = 0;
  let operatorAccountID: number;
  let operator: string;

  const bVerify = true;

  const verify = async () => {
    if (bVerify) {
      await exchangeTestUtil.submitPendingBlocks();
    }
  };

  before(async () => {
    exchangeTestUtil = new ExchangeTestUtil();
    await exchangeTestUtil.initialize(accounts);
    exchangeID = 1;
  });

  after(async () => {
    await exchangeTestUtil.stop();
  });

  beforeEach(async () => {
    // Fresh Exchange for each test
    exchangeID = await exchangeTestUtil.createExchange(exchangeTestUtil.testContext.stateOwners[0]);
    operatorAccountID = await exchangeTestUtil.getActiveOperator(exchangeID);
    operator = exchangeTestUtil.getAccount(operatorAccountID).owner;
  });

  describe("Trade", function() {
    this.timeout(0);

    it("Matchable (orderA < orderB)", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("5", "ether")),
          amountB: new BN(web3.utils.toWei("45", "ether"))
        },
        expected: {
          orderA: {
            filledFraction: 0.5,
            spread: new BN(web3.utils.toWei("5", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Matchable (orderA > orderB)", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "LRC",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("101", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "LRC",
          amountS: new BN(web3.utils.toWei("20", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether"))
        },
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("1", "ether"))
          },
          orderB: { filledFraction: 0.5 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("feeBips < maxFeeBips", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "LRC",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("101", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          maxFeeBips: 18,
          feeBips: 18
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "LRC",
          amountS: new BN(web3.utils.toWei("20", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          maxFeeBips: 18,
          feeBips: 18
        },
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("1", "ether"))
          },
          orderB: { filledFraction: 0.5 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Unmatchable", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("90", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        expected: {
          orderA: {
            filledFraction: 0.0,
            spread: new BN(web3.utils.toWei("10", "ether")).neg()
          },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("Specific taker (correct)", async () => {
      const ring: SpotTrade = {
        orderA: {
          owner: exchangeTestUtil.testContext.orderOwners[0],
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          taker: exchangeTestUtil.testContext.orderOwners[1]
        },
        orderB: {
          owner: exchangeTestUtil.testContext.orderOwners[1],
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("10", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        expected: {
          orderA: { filledFraction: 1.0, spread: new BN(0) },
          orderB: { filledFraction: 1.0 }
        }
      };
      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Specific taker (incorrect)", async () => {
      const ring: SpotTrade = {
        orderA: {
          owner: exchangeTestUtil.testContext.orderOwners[0],
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          taker: exchangeTestUtil.testContext.orderOwners[0]
        },
        orderB: {
          owner: exchangeTestUtil.testContext.orderOwners[1],
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("10", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        expected: {
          orderA: { filledFraction: 1.0, spread: new BN(0) },
          orderB: { filledFraction: 1.0 }
        }
      };
      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("No funds available", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("5", "ether")),
          amountB: new BN(web3.utils.toWei("45", "ether")),
          balanceS: new BN(0)
        },
        expected: {
          orderA: { filledFraction: 0.0, spread: new BN(0) },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Insufficient funds available (buy order)", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          balanceS: new BN(web3.utils.toWei("40", "ether")),
          fillAmountBorS: true
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("20", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether"))
        },
        expected: {
          orderA: { filledFraction: 0.4, spread: new BN(0) },
          orderB: { filledFraction: 0.2 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Insufficient funds available (sell order)", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          balanceS: new BN(web3.utils.toWei("40", "ether")),
          fillAmountBorS: false
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("20", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether"))
        },
        expected: {
          orderA: { filledFraction: 0.4, spread: new BN(0) },
          orderB: { filledFraction: 0.2 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Self-trading", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("105", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          balanceS: new BN(web3.utils.toWei("105", "ether")),
          balanceB: new BN(web3.utils.toWei("10", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("10", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("5", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);

      ring.orderB.accountID = ring.orderA.accountID;
      ring.orderB.signature = undefined;
      exchangeTestUtil.signOrder(ring.orderB);

      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("selling token with decimals == 0", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "INDA",
          tokenB: "WETH",
          amountS: new BN(60),
          amountB: new BN(web3.utils.toWei("5", "ether"))
        },
        orderB: {
          tokenS: "WETH",
          tokenB: "INDA",
          amountS: new BN(web3.utils.toWei("2.5", "ether")),
          amountB: new BN(25)
        },
        expected: {
          orderA: { filledFraction: 0.5, spread: new BN(5) },
          orderB: { filledFraction: 1.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("fillAmountB rounding error > 0.1%", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "INDA",
          tokenB: "INDB",
          amountS: new BN(200),
          amountB: new BN(2000),
          fillAmountBorS: true
        },
        orderB: {
          tokenS: "INDB",
          tokenB: "INDA",
          amountS: new BN(2000),
          amountB: new BN(200),
          balanceS: new BN(1999),
          fillAmountBorS: false
        },
        expected: {
          orderA: { filledFraction: 0.0 },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("fillAmountB is 0 because of rounding error", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "INDA",
          tokenB: "INDB",
          amountS: new BN(1),
          amountB: new BN(10)
        },
        orderB: {
          tokenS: "INDB",
          tokenB: "INDA",
          amountS: new BN(10),
          amountB: new BN(1),
          balanceS: new BN(5)
        },
        expected: {
          orderA: { filledFraction: 0.0, spread: new BN(0) },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("operator == order owner", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "LRC",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        orderB: {
          tokenS: "LRC",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      exchangeTestUtil.setActiveOperator(ring.orderB.accountID);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("orderA.owner == orderB.owner == operator", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("105", "ether")),
          amountB: new BN(web3.utils.toWei("10", "ether")),
          balanceS: new BN(web3.utils.toWei("105", "ether")),
          balanceB: new BN(web3.utils.toWei("10", "ether"))
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("10", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        tokenID: await exchangeTestUtil.getTokenIdFromNameOrAddress("WETH"),
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("5", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);

      ring.orderB.accountID = ring.orderA.accountID;
      ring.orderB.signature = undefined;
      exchangeTestUtil.signOrder(ring.orderB);

      await exchangeTestUtil.sendRing(ring);

      exchangeTestUtil.setActiveOperator(ring.orderA.accountID);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("tokenS/tokenB mismatch", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        orderB: {
          tokenS: "LRC",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("tokenS == tokenB", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        orderB: {
          tokenS: "WETH",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("Wrong order signature", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "LRC",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          signature: { Rx: "4564565564545", Ry: "456445648974", s: "445644894" }
        },
        orderB: {
          tokenS: "LRC",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("validUntil < now", async () => {
      const ring: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          validUntil: 1
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("200", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether"))
        },
        expected: {
          orderA: { filledFraction: 0.0, spread: new BN(0) },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ring);
      await exchangeTestUtil.sendRing(ring);

      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("Multiple rings", async () => {
      const ringA: SpotTrade = {
        orderA: {
          tokenS: "ETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("3", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0]
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("2", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1]
        },
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("1", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };
      const ringB: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("110", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[2]
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("200", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[3]
        },
        expected: {
          orderA: {
            filledFraction: 1.0,
            spread: new BN(web3.utils.toWei("10", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };

      await exchangeTestUtil.setupRing(ringA);
      await exchangeTestUtil.setupRing(ringB);
      await exchangeTestUtil.sendRing(ringA);
      await exchangeTestUtil.sendRing(ringB);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Order filled in multiple rings (order.storageID == storage.storageID)", async () => {
      const order: OrderInfo = {
        tokenS: "ETH",
        tokenB: "GTO",
        amountS: new BN(web3.utils.toWei("10", "ether")),
        amountB: new BN(web3.utils.toWei("100", "ether")),
        owner: exchangeTestUtil.testContext.orderOwners[0]
      };
      await exchangeTestUtil.setupOrder(order, 0);

      const ringA: SpotTrade = {
        orderA: order,
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("60", "ether")),
          amountB: new BN(web3.utils.toWei("6", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1]
        },
        expected: {
          orderA: {
            filledFraction: 0.6,
            spread: new BN(web3.utils.toWei("0", "ether"))
          },
          orderB: { filledFraction: 1.0 }
        }
      };
      const ringB: SpotTrade = {
        orderA: order,
        orderB: {
          tokenS: "GTO",
          tokenB: "ETH",
          amountS: new BN(web3.utils.toWei("120", "ether")),
          amountB: new BN(web3.utils.toWei("12", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[2]
        },
        expected: {
          orderA: {
            filledFraction: 0.4,
            spread: new BN(web3.utils.toWei("0", "ether"))
          },
          orderB: { filledFraction: 0.33333 }
        }
      };

      await exchangeTestUtil.setupRing(ringA, false, true);
      await exchangeTestUtil.setupRing(ringB, false, true);
      await exchangeTestUtil.sendRing(ringA);
      await exchangeTestUtil.sendRing(ringB);

      await exchangeTestUtil.submitTransactions();

      await verify();
    });

    it("Reused storageID (storage.data > order.amount)", async () => {
      const storageID = 79;
      const ringA: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1000", "ether")),
          amountB: new BN(web3.utils.toWei("2000", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          storageID
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("2000", "ether")),
          amountB: new BN(web3.utils.toWei("1000", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1]
        },
        expected: {
          orderA: { filledFraction: 1.0, spread: new BN(0) },
          orderB: { filledFraction: 1.0 }
        }
      };
      const ringB: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          storageID
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("200", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[3]
        },
        expected: {
          orderA: { filledFraction: 0.0, spread: new BN(0) },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ringA);
      await exchangeTestUtil.sendRing(ringA);
      await exchangeTestUtil.submitTransactions();
      await verify();

      await exchangeTestUtil.setupRing(ringB);
      await exchangeTestUtil.sendRing(ringB);
      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });

    it("Cancelled storageID (order.storageID < storage.storageID)", async () => {
      const storageID = 8;
      const ringA: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          storageID: storageID + 2 ** Constants.BINARY_TREE_DEPTH_STORAGE
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("200", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[1]
        },
        expected: {
          orderA: { filledFraction: 1.0, spread: new BN(0) },
          orderB: { filledFraction: 1.0 }
        }
      };
      const ringB: SpotTrade = {
        orderA: {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("100", "ether")),
          amountB: new BN(web3.utils.toWei("200", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[0],
          storageID
        },
        orderB: {
          tokenS: "GTO",
          tokenB: "WETH",
          amountS: new BN(web3.utils.toWei("200", "ether")),
          amountB: new BN(web3.utils.toWei("100", "ether")),
          owner: exchangeTestUtil.testContext.orderOwners[3]
        },
        expected: {
          orderA: { filledFraction: 0.0, spread: new BN(0) },
          orderB: { filledFraction: 0.0 }
        }
      };

      await exchangeTestUtil.setupRing(ringA);
      await exchangeTestUtil.sendRing(ringA);
      await exchangeTestUtil.submitTransactions();
      await verify();

      await exchangeTestUtil.setupRing(ringB);
      await exchangeTestUtil.sendRing(ringB);
      await expectThrow(exchangeTestUtil.submitTransactions(), "invalid block");
    });
  });
});
