/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "CCGateways.h"

/*
 prevent duplicate bindtxid via mempool scan
 wait for notarization for oraclefeed and validation of gatewaysdeposit
 debug multisig and do partial signing
 validation
 
 string oracles
 */

/*
 Uses MofN CC's normal msig handling to create automated deposits -> token issuing. And partial signing by the selected pubkeys for releasing the funds. A user would be able to select which pubkeys to use to construct the automated deposit/redeem multisigs.
 
 the potential pubkeys to be used would be based on active oracle data providers with recent activity.
 
 bind asset <-> KMD gateway deposit address
 KMD deposit -> globally spendable marker utxo
 spend marker utxo and spend linked/locked asset to user's CC address
 
 redeem -> asset to global CC address with withdraw address -> gateway spendable marker utxo
 spend market utxo and withdraw from gateway deposit address
 
 rpc calls:
 GatewayList
 GatewayInfo bindtxid
 GatewayBind coin tokenid M N pubkey(s)
 external: deposit to depositaddr with claimpubkey
 GatewayDeposit coin tokenid external.deposittxid -> markertxid
 GatewayClaim coin tokenid external.deposittxid markertxid -> spend marker and deposit asset
 
 GatewayWithdraw coin tokenid withdrawaddr
 external: do withdraw to withdrawaddr and spend marker, support for partial signatures and autocomplete
 
 deposit addr can be 1 to MofN pubkeys
 1:1 gateway with native coin
 
 In order to create a new gateway it is necessary to follow some strict steps.
 1. create a token with the max possible supply that will be issued
 2. transfer 100% of them to the gateways CC's global pubkey's asset CC address. (yes it is a bit confusing)
 3. create an oracle with the identical name, ie. KMD and format must start with Ihh (height, blockhash, merkleroot)
 4. register a publisher and fund it with a subscribe. there will be a special client app that will automatically publish the merkleroots.
 5. Now a gatewaysbind can bind an external coin to an asset, along with the oracle for the merkleroots. the txid from the bind is used in most of the other gateways CC calls
 
 usage:
 ./c tokencreate KMD 1000000 KMD_equivalent_token_for_gatewaysCC
 a7398a8748354dd0a3f8d07d70e65294928ecc3674674bb2d9483011ccaa9a7a
 
 transfer to gateways pubkey: 03ea9c062b9652d8eff34879b504eda0717895d27597aaeb60347d65eed96ccb40 RDMqGyREkP1Gwub1Nr5Ye8a325LGZsWBCb
 ./c tokentransfer a7398a8748354dd0a3f8d07d70e65294928ecc3674674bb2d9483011ccaa9a7a 03ea9c062b9652d8eff34879b504eda0717895d27597aaeb60347d65eed96ccb40 100000000000000
 2206fc39c0f384ca79819eb491ddbf889642cbfe4d0796bb6a8010ed53064a56
 
 ./c oraclescreate KMD blockheaders Ihh
 1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808
 
 ./c oraclesregister 1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808 1000000
 83b59eac238cbe54616ee13b2fdde85a48ec869295eb04051671a1727c9eb402
 
 ./c oraclessubscribe 1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808 02ebc786cb83de8dc3922ab83c21f3f8a2f3216940c3bf9da43ce39e2a3a882c92 1000
 f9499d8bb04ffb511fcec4838d72e642ec832558824a2ce5aed87f1f686f8102
 
 gatewaysbind tokenid oracletxid coin tokensupply M N pubkey(s)
 ./c gatewaysbind a7398a8748354dd0a3f8d07d70e65294928ecc3674674bb2d9483011ccaa9a7a 1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808 KMD 100000000000000 1 1 02ebc786cb83de8dc3922ab83c21f3f8a2f3216940c3bf9da43ce39e2a3a882c92
 e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e
 
 ./c gatewaysinfo e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e
 {
 "result": "success",
 "name": "Gateways",
 "pubkey": "02ebc786cb83de8dc3922ab83c21f3f8a2f3216940c3bf9da43ce39e2a3a882c92",
 "coin": "KMD",
 "oracletxid": "1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808",
 "taddr": 0,
 "prefix": 60,
 "prefix2": 85,
 "deposit": "",
 "tokenid": "a7398a8748354dd0a3f8d07d70e65294928ecc3674674bb2d9483011ccaa9a7a",
 "totalsupply": "1000000.00000000",
 "remaining": "1000000.00000000",
 "issued": "0.00000000"
 }
 
 To make a gateway deposit, send the funds to the "deposit" address, along with any amount to the same pubkey address you want to get the assetized KMD to appear in.
 
 0223d114dededb04f253816d6ad0ce78dd08c617c94ce3c53bf50dc74a5157bef8 pubkey for RFpxgqff7FDHFuHa3jSX5NzqqWCcELz8ha
 ./komodo-cli z_sendmany "<funding addr>" '[{"address":"RFpxgqff7FDHFuHa3jSX5NzqqWCcELz8ha","amount":0.0001},{"address":"RHV2As4rox97BuE3LK96vMeNY8VsGRTmBj","amount":7.6999}]'
 bc41a00e429db741c3199f17546a48012fd3b7eea45dfc6bc2f5228278133009 height.1003776 merkle.90aedc2f19200afc9aca2e351438d011ebae8264a58469bf225883045f61917f
 
 ./komodo-cli gettxoutproof '["bc41a00e429db741c3199f17546a48012fd3b7eea45dfc6bc2f5228278133009"]'
 04000000232524ea04b54489eb222f8b3f566ed35504e3050488a63f0ab7b1c2030000007f91615f04835822bf6984a56482aeeb11d03814352eca9afc0a20192fdcae900000000000000000000000000000000000000000000000000000000000000000268e965ba608071d0800038e16762900000000000000000000000000000000000000000042008dd6fd4005016b4292b05df6dfd316c458c53e28fb2befb96e4870a40c6c04e733d75d8a7a18cce34fe326005efdc403bfa4644e30eeafdaeff34419edc591299e6cc5933cb2eeecbab5c4dfe97cd413b75215999a3dd02b540373581e81d8512bff1640590a6b4da4aaa9b8adc0102c38ca0022daed997b53ed192ba326e212fba5e505ce29e3ad149cef7f48d0e00948a1acd81731d84008760759211eb4abbc7b037939a7964182edb59cf9065357e864188ee5fc7316e8796963036bb99eeb9f06c95d64f78749ecec7181c12eb5d83a3b9b1c1e8a0aae9a20ce04a250b28216620bfc99bb81a6de4db80b93a5aea916de97c1a272e26644abdd683f19c5e3174a2e4513ed767d8f11a4c3074295f697839c5d9139676a813451cc7da38f68cbae5d990a79075f98903233ca04fe1b4b099e433585e5adcc45d41d54a9c648179297359c75950a5e574f13f70b728bbbf552770256315cd0a00139d6ab6934cb5ed70a4fc01a92611b096dd0028f17f4cc687b75f37dca530aa47a18321c50528dbd9272eabb3e13a87021a05918a6d2627e2caba6d7cf1a9f0b831ea3337b9a6af92746d83140078d60c72b6beacf91c9e68a34cee209e08670be1d17ff8d80b7a2285b1325461a2e33f2ee675593f1900e066a5d212615cd8da18749b0e684eee73edcc9031709be715b889c6d015cf4bd4ad5ab7e21bd3492c208930a54d353ef36a437f507ead38855633c1b88d060d9e4221ca8ce2f698e8a6ae0d41e9ace3cbd401f1e0f07650e9c126d4ef20278c8be6e85c7637513643f8d02d7ad64c09da11c16429d60e5160c345844b8158ece62794e8ad280d4e4664150e74978609ece431e51a9f9e1ce8aa49c16f36c7fd12b71acc42d893e18476b8b1e144a8175519612efc93e0aecc61f3b21212c958b0e2331d76aaa62faf11a58fe2bd91ab9ab01b906406c9bbc02df2a106e67182aae0a20b538bf19f09c57f9de5e198ba254580fb1b11e22ad526550093420cb7c68628d4c3ad329c8acc6e219093d277810ed016b6099b7e3781de412a22dacedaa2acf29e8062debcd85c7b9529a20b2782a2470763ac27cf89611a527d43ac89b8063ffb93b6ed993425194f8ee821a8493a563072c896f9584f95db28e3f2fc5fb4a6f3c39d615cd563641717cd50afb73ed3989cbf504b2043882993ce9575f56402534173b1396fbc13df80920b46788ae340ad5a91f25177cc74aa69024d76f56166199d2e4d50a053555256c4e3137ea1cee1130e916a88b6ee5cf2c85652fb8824d5dacfa485e3ef6190591ac0c2fcacc4fc7deb65aca4b0b89b76e35a46b0627e2e967cc63a5d606a984c8e63eabb98fde3e69114340ae524c974cb936e57690e98a7a74533f6f7d1d0496976496b54d14a8163efb32b70dfbb79d80a3022c4f53571c08bf044270565716b435084376714b224ab23e9817c05af8223723afc0577af5c8fc28f71036ca82528aaa4ca9bcd18a50e25d2a528f183d3a2074d968d170876d8dce434c5937261b55173ab87e03d5632ca0834fdc5387c15ab3a17d75c0f274004f289ff1bf7d14e97fdf4172eb49adfb418cc2f4794806ae7c0111c97df4d65d38679ec93fea3ef738ed565e8906a8fe1861cafe3938c772fedcfab40159938e06ef414fd299f2355c6d3369bc1bd3c4db64ce205f0a1b70a40030f505b736e28230de82e97776b5ee7b10708bb3020d28cec7a8e124549ec80c547ac4e7b52bf397c72bcfce30820554ab8fb4d1f73b209bc32a0e7e878843cdbf5f01222728ccea7e6ab7cb5e3fee3234f5b85d1985f91492f6ceaa6454a658dab5074f163ce26ed753137fa61c940679de13bd7b212cd3cf2b334f5201cecbc7473342bd7a239e09169bccd56d03000000037a9068df0625e548e71263c8361b4e904c998378f6b9e32729c3f19b10ad752e093013788222f5c26bfc5da4eeb7d32f01486a54179f19c341b79d420ea041bc8878d22fad4692b2d609c3cf190903874d3682a714c7483518c9392e07c25035010b
 
 ./komodo-cli getrawtransaction bc41a00e429db741c3199f17546a48012fd3b7eea45dfc6bc2f5228278133009
 010000000149964cdcd17fe9b1cae4d0f3b5f5db301d9b4f54099fdf4d34498df281757094010000006a4730440220594f3a630dd73c123f44621aa8bb9968ab86734833453dd479af6d79ae6f584202207bb5e35f13b337ccc8a88d9a006c8c5ddb016c0a6f4f2dc44357a8128623d85d01210223154bf53cd3a75e64d86697070d6437c8f0010a09c1df35b659e31ce3d79b5dffffffff0310270000000000001976a91447d2e323a14b0c3be08698aa46a9b91489b189d688ac701de52d000000001976a91459fdba29ea85c65ad90f6d38f7a6646476b26b1688acb0a86a00000000001976a914f9a9daf5519dae38b8b61d945f075da895df441d88ace18d965b
 
 gatewaysdeposit bindtxid height coin cointxid claimvout deposithex proof destpub amount
 ./komodo-cli -ac_name=AT5 gatewaysdeposit e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e 1003776 KMD bc41a00e429db741c3199f17546a48012fd3b7eea45dfc6bc2f5228278133009 0 010000000149964cdcd17fe9b1cae4d0f3b5f5db301d9b4f54099fdf4d34498df281757094010000006a4730440220594f3a630dd73c123f44621aa8bb9968ab86734833453dd479af6d79ae6f584202207bb5e35f13b337ccc8a88d9a006c8c5ddb016c0a6f4f2dc44357a8128623d85d01210223154bf53cd3a75e64d86697070d6437c8f0010a09c1df35b659e31ce3d79b5dffffffff0310270000000000001976a91447d2e323a14b0c3be08698aa46a9b91489b189d688ac701de52d000000001976a91459fdba29ea85c65ad90f6d38f7a6646476b26b1688acb0a86a00000000001976a914f9a9daf5519dae38b8b61d945f075da895df441d88ace18d965b 04000000232524ea04b54489eb222f8b3f566ed35504e3050488a63f0ab7b1c2030000007f91615f04835822bf6984a56482aeeb11d03814352eca9afc0a20192fdcae900000000000000000000000000000000000000000000000000000000000000000268e965ba608071d0800038e16762900000000000000000000000000000000000000000042008dd6fd4005016b4292b05df6dfd316c458c53e28fb2befb96e4870a40c6c04e733d75d8a7a18cce34fe326005efdc403bfa4644e30eeafdaeff34419edc591299e6cc5933cb2eeecbab5c4dfe97cd413b75215999a3dd02b540373581e81d8512bff1640590a6b4da4aaa9b8adc0102c38ca0022daed997b53ed192ba326e212fba5e505ce29e3ad149cef7f48d0e00948a1acd81731d84008760759211eb4abbc7b037939a7964182edb59cf9065357e864188ee5fc7316e8796963036bb99eeb9f06c95d64f78749ecec7181c12eb5d83a3b9b1c1e8a0aae9a20ce04a250b28216620bfc99bb81a6de4db80b93a5aea916de97c1a272e26644abdd683f19c5e3174a2e4513ed767d8f11a4c3074295f697839c5d9139676a813451cc7da38f68cbae5d990a79075f98903233ca04fe1b4b099e433585e5adcc45d41d54a9c648179297359c75950a5e574f13f70b728bbbf552770256315cd0a00139d6ab6934cb5ed70a4fc01a92611b096dd0028f17f4cc687b75f37dca530aa47a18321c50528dbd9272eabb3e13a87021a05918a6d2627e2caba6d7cf1a9f0b831ea3337b9a6af92746d83140078d60c72b6beacf91c9e68a34cee209e08670be1d17ff8d80b7a2285b1325461a2e33f2ee675593f1900e066a5d212615cd8da18749b0e684eee73edcc9031709be715b889c6d015cf4bd4ad5ab7e21bd3492c208930a54d353ef36a437f507ead38855633c1b88d060d9e4221ca8ce2f698e8a6ae0d41e9ace3cbd401f1e0f07650e9c126d4ef20278c8be6e85c7637513643f8d02d7ad64c09da11c16429d60e5160c345844b8158ece62794e8ad280d4e4664150e74978609ece431e51a9f9e1ce8aa49c16f36c7fd12b71acc42d893e18476b8b1e144a8175519612efc93e0aecc61f3b21212c958b0e2331d76aaa62faf11a58fe2bd91ab9ab01b906406c9bbc02df2a106e67182aae0a20b538bf19f09c57f9de5e198ba254580fb1b11e22ad526550093420cb7c68628d4c3ad329c8acc6e219093d277810ed016b6099b7e3781de412a22dacedaa2acf29e8062debcd85c7b9529a20b2782a2470763ac27cf89611a527d43ac89b8063ffb93b6ed993425194f8ee821a8493a563072c896f9584f95db28e3f2fc5fb4a6f3c39d615cd563641717cd50afb73ed3989cbf504b2043882993ce9575f56402534173b1396fbc13df80920b46788ae340ad5a91f25177cc74aa69024d76f56166199d2e4d50a053555256c4e3137ea1cee1130e916a88b6ee5cf2c85652fb8824d5dacfa485e3ef6190591ac0c2fcacc4fc7deb65aca4b0b89b76e35a46b0627e2e967cc63a5d606a984c8e63eabb98fde3e69114340ae524c974cb936e57690e98a7a74533f6f7d1d0496976496b54d14a8163efb32b70dfbb79d80a3022c4f53571c08bf044270565716b435084376714b224ab23e9817c05af8223723afc0577af5c8fc28f71036ca82528aaa4ca9bcd18a50e25d2a528f183d3a2074d968d170876d8dce434c5937261b55173ab87e03d5632ca0834fdc5387c15ab3a17d75c0f274004f289ff1bf7d14e97fdf4172eb49adfb418cc2f4794806ae7c0111c97df4d65d38679ec93fea3ef738ed565e8906a8fe1861cafe3938c772fedcfab40159938e06ef414fd299f2355c6d3369bc1bd3c4db64ce205f0a1b70a40030f505b736e28230de82e97776b5ee7b10708bb3020d28cec7a8e124549ec80c547ac4e7b52bf397c72bcfce30820554ab8fb4d1f73b209bc32a0e7e878843cdbf5f01222728ccea7e6ab7cb5e3fee3234f5b85d1985f91492f6ceaa6454a658dab5074f163ce26ed753137fa61c940679de13bd7b212cd3cf2b334f5201cecbc7473342bd7a239e09169bccd56d03000000037a9068df0625e548e71263c8361b4e904c998378f6b9e32729c3f19b10ad752e093013788222f5c26bfc5da4eeb7d32f01486a54179f19c341b79d420ea041bc8878d22fad4692b2d609c3cf190903874d3682a714c7483518c9392e07c25035010b 0223d114dededb04f253816d6ad0ce78dd08c617c94ce3c53bf50dc74a5157bef8 7.6999
 -> 9d80ea79a65aaa0d464f8b762356fa01047e16e9793505a22ca04559f81a6eb6
 
 to get the merkleroots onchain, from the multisig signers nodes run the oraclefeed program with acname oracletxid pubkey Ihh
 ./oraclefeed AT5 1f1aefcca2bdea8196cfd77337fb21de22d200ddea977c2f9e8742c55829d808 02ebc786cb83de8dc3922ab83c21f3f8a2f3216940c3bf9da43ce39e2a3a882c92 Ihh
 
 gatewaysclaim bindtxid coin deposittxid destpub amount
 ./c gatewaysclaim e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e KMD 9d80ea79a65aaa0d464f8b762356fa01047e16e9793505a22ca04559f81a6eb6 0223d114dededb04f253816d6ad0ce78dd08c617c94ce3c53bf50dc74a5157bef8 7.6999
 
 now the asset is in the pubkey's asset address!
 it can be used, traded freely and any node who has the asset can do a gatewayswithdraw
 
 gatewayswithdraw bindtxid coin withdrawpub amount
 ./c gatewayswithdraw e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e KMD 03b7621b44118017a16043f19b30cc8a4cfe068ac4e42417bae16ba460c80f3828 1
 ef3cc452da006eb2edda6b6ed3d3347664be51260f3e91f59ec44ec9701367f0
 
 Now there is a withdraw pending, so it needs to be processed by the signing nodes on the KMD side
 
 gatewayspending bindtxid coin
 gatewayspending will display all pending withdraws and if it is done on one of the msigpubkeys, then it will queue it for processing
 ./c gatewayspending e6c99f79d4afb216aa8063658b4222edb773dd24bb0f8e91bd4ef341f3e47e5e KMD
 
 
 Implementation Issues:
    -- When thinking about validation, it is clear that we cant use EVAL_ASSETS for the locked coins as there wont be any enforcement of the gateways locking. 
    -- This means we need a way to transfer assets into gateways outputs and back. It seems a tokenconvert rpc will be needed and hopefully that will be enough to make it all work properly.
    ++ The use of tokenconvert has been changed to the use of the new Tokens contract which can enforce other contracts validation by forwarding eval->validate call to GatewaysValidate
	++ So all tokens remain within that Tokens contract eval code.
 
    -- Care must be taken so that tokens are not lost and can be converted back.
    -- This changes the usage to require tokenconvert before doing the bind and also tokenconvert before doing a withdraw. EVAL_GATEWAYS has evalcode of 241
	++ tokenconvert now returns 'not implemented', no need to use it at all.
 
    -- The gatewaysclaim automatically converts the deposit amount of tokens back to EVAL_ASSETS.
    ++ The gatewaysclaim automatically transfers the deposit amount of tokens to depositor's address (within EVAL_TOKENS).

 */
// start of consensus code

CScript EncodeGatewaysBindOpRet(uint8_t funcid,uint256 tokenid,std::string coin,int64_t totalsupply,uint256 oracletxid,uint8_t M,uint8_t N,std::vector<CPubKey> gatewaypubkeys,uint8_t taddr,uint8_t prefix,uint8_t prefix2)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS; struct CCcontract_info *cp,C; CPubKey gatewayspk;
    std::vector<CPubKey> pubkeys;
    
    cp = CCinit(&C,EVAL_GATEWAYS);
    gatewayspk = GetUnspendable(cp,0);
    pubkeys.push_back(gatewayspk);
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << coin << totalsupply << oracletxid << M << N << gatewaypubkeys << taddr << prefix << prefix2);
    return(EncodeTokenOpRet(tokenid,pubkeys,opret));
}

uint8_t DecodeGatewaysBindOpRet(char *depositaddr,const CScript &scriptPubKey,uint256 &tokenid,std::string &coin,int64_t &totalsupply,uint256 &oracletxid,uint8_t &M,uint8_t &N,std::vector<CPubKey> &gatewaypubkeys,uint8_t &taddr,uint8_t &prefix,uint8_t &prefix2)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f,tokenevalcode;
    std::vector<uint8_t> vOpretExtra; std::vector<CPubKey> pubkeys;

    if (DecodeTokenOpRet(scriptPubKey,tokenevalcode,tokenid,pubkeys,vOpretExtra)!=0 && tokenevalcode==EVAL_TOKENS && vOpretExtra.size()>0)
    {
        if (!E_UNMARSHAL(vOpretExtra, { ss >> vopret; })) return (0);
    }
    else GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    depositaddr[0] = 0;
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> coin; ss >> totalsupply; ss >> oracletxid; ss >> M; ss >> N; ss >> gatewaypubkeys; ss >> taddr; ss >> prefix; ss >> prefix2;) != 0 )
    {
        if ( prefix == 60 )
        {
            if ( N > 1 )
            {
                strcpy(depositaddr,CBitcoinAddress(CScriptID(GetScriptForMultisig(M,gatewaypubkeys))).ToString().c_str());
                //fprintf(stderr,"f.%c M.%d of N.%d size.%d -> %s\n",f,M,N,(int32_t)gatewaypubkeys.size(),depositaddr);
            } else Getscriptaddress(depositaddr,CScript() << ParseHex(HexStr(gatewaypubkeys[0])) << OP_CHECKSIG);
        }
        else
        {
            fprintf(stderr,"need to generate non-KMD addresses prefix.%d\n",prefix);
        }
        return(f);
    } else fprintf(stderr,"error decoding bind opret\n");
    return(0);
}

CScript EncodeGatewaysDepositOpRet(uint8_t funcid,uint256 bindtxid,std::string refcoin,std::vector<CPubKey> publishers,std::vector<uint256>txids,int32_t height,uint256 cointxid,int32_t claimvout,std::string deposithex,std::vector<uint8_t>proof,CPubKey destpub,int64_t amount)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << refcoin << bindtxid << publishers << txids << height << cointxid << claimvout << deposithex << proof << destpub << amount);
    return(opret);
}

uint8_t DecodeGatewaysDepositOpRet(const CScript &scriptPubKey,uint256 &bindtxid,std::string &refcoin,std::vector<CPubKey>&publishers,std::vector<uint256>&txids,int32_t &height,uint256 &cointxid, int32_t &claimvout,std::string &deposithex,std::vector<uint8_t> &proof,CPubKey &destpub,int64_t &amount)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f;
    GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> refcoin; ss >> bindtxid; ss >> publishers; ss >> txids; ss >> height; ss >> cointxid; ss >> claimvout; ss >> deposithex; ss >> proof; ss >> destpub; ss >> amount) != 0 )
    {
        return(f);
    }
    return(0);
}

CScript EncodeGatewaysClaimOpRet(uint8_t funcid,uint256 tokenid,uint256 bindtxid,std::string refcoin,uint256 deposittxid,CPubKey destpub,int64_t amount)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS; struct CCcontract_info *cp,C; CPubKey gatewayspk;
    std::vector<CPubKey> pubkeys;

    pubkeys.push_back(destpub);
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << bindtxid << refcoin << deposittxid << destpub << amount);        
    return(EncodeTokenOpRet(tokenid,pubkeys,opret));
}

uint8_t DecodeGatewaysClaimOpRet(const CScript &scriptPubKey,uint256 &tokenid,uint256 &bindtxid,std::string &refcoin,uint256 &deposittxid,CPubKey &destpub,int64_t &amount)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f,tokenevalcode;
    std::vector<CPubKey> pubkeys; std::vector<uint8_t> vOpretExtra;

    if (DecodeTokenOpRet(scriptPubKey,tokenevalcode,tokenid,pubkeys,vOpretExtra)!=0 && tokenevalcode==EVAL_TOKENS && vOpretExtra.size()>0)
    {
        if (!E_UNMARSHAL(vOpretExtra, { ss >> vopret; })) return (0);
    }
    else GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> e; ss >> f; ss >> bindtxid; ss >> refcoin; ss >> deposittxid; ss >> destpub; ss >> amount) != 0 )
    {
        return(f);
    }
    return(0);
}

CScript EncodeGatewaysWithdrawOpRet(uint8_t funcid,uint256 tokenid,uint256 bindtxid,std::string refcoin,CPubKey withdrawpub,int64_t amount)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS; struct CCcontract_info *cp,C; CPubKey gatewayspk;
    std::vector<CPubKey> pubkeys;

    cp = CCinit(&C,EVAL_GATEWAYS);
    gatewayspk = GetUnspendable(cp,0);
    pubkeys.push_back(gatewayspk);
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << bindtxid << refcoin << withdrawpub << amount);        
    return(EncodeTokenOpRet(tokenid,pubkeys,opret));
}

uint8_t DecodeGatewaysWithdrawOpRet(const CScript &scriptPubKey, uint256& tokenid, uint256 &bindtxid, std::string &refcoin, CPubKey &withdrawpub, int64_t &amount)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f,tokenevalcode;
    std::vector<CPubKey> pubkeys; std::vector<uint8_t> vOpretExtra;

    if (DecodeTokenOpRet(scriptPubKey,tokenevalcode,tokenid,pubkeys,vOpretExtra)!=0 && tokenevalcode==EVAL_TOKENS && vOpretExtra.size()>0)
    {
        if (!E_UNMARSHAL(vOpretExtra, { ss >> vopret; })) return (0);
    }
    else GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> e; ss >> f; ss >> bindtxid; ss >> refcoin; ss >> withdrawpub; ss >> amount) != 0 )
    {
        return(f);
    }
    return(0);
}

CScript EncodeGatewaysPartialOpRet(uint8_t funcid, uint256 withdrawtxid,std::string refcoin,uint8_t K, CPubKey signerpk,std::string hex)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << withdrawtxid << refcoin  << K << signerpk << hex);        
    return(opret);
}

uint8_t DecodeGatewaysPartialOpRet(const CScript &scriptPubKey,uint256 &withdrawtxid,std::string &refcoin,uint8_t &K,CPubKey &signerpk,std::string &hex)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f;
    GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> withdrawtxid; ss >> refcoin; ss >> K; ss >> signerpk; ss >> hex) != 0 )
    {
        return(f);
    }
    return(0);
}

CScript EncodeGatewaysCompleteSigningOpRet(uint8_t funcid,uint256 withdrawtxid,std::string refcoin,uint8_t K,std::string hex)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << withdrawtxid << refcoin << K << hex);        
    return(opret);
}

uint8_t DecodeGatewaysCompleteSigningOpRet(const CScript &scriptPubKey,uint256 &withdrawtxid,std::string &refcoin,uint8_t &K,std::string &hex)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f;
    GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> withdrawtxid; ss >> refcoin; ss >> K; ss >> hex) != 0 )
    {
        return(f);
    }
    return(0);
}

CScript EncodeGatewaysMarkDoneOpRet(uint8_t funcid,uint256 withdrawtxid,std::string refcoin,uint256 completetxid)
{
    CScript opret; uint8_t evalcode = EVAL_GATEWAYS;
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << withdrawtxid << refcoin << completetxid);        
    return(opret);
}

uint8_t DecodeGatewaysMarkDoneOpRet(const CScript &scriptPubKey, uint256 &withdrawtxid, std::string &refcoin, uint256 &completetxid)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f;
    GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && E_UNMARSHAL(vopret,ss >> e; ss >> f; ss >> withdrawtxid; ss >> refcoin; ss >> completetxid;) != 0 )
    {
        return(f);
    }
    return(0);
}

uint8_t DecodeGatewaysOpRet(const CScript &scriptPubKey)
{
    std::vector<uint8_t> vopret; uint8_t *script,e,f,tokenevalcode;
    std::vector<CPubKey> pubkeys; std::vector<uint8_t> vOpretExtra; uint256 tokenid;

    if (DecodeTokenOpRet(scriptPubKey,tokenevalcode,tokenid,pubkeys,vOpretExtra)!=0 && tokenevalcode==EVAL_TOKENS && vOpretExtra.size()>0)
    {
        if (!E_UNMARSHAL(vOpretExtra, { ss >> vopret; })) return (0);
    }
    else GetOpReturnData(scriptPubKey, vopret);
    script = (uint8_t *)vopret.data();
    if ( vopret.size() > 2 && script[0] == EVAL_GATEWAYS)
    {
        f=script[1];
        if (f == 'B' || f == 'D' || f == 'C' || f == 'W' || f == 'P' || f == 'S' || f == 'M')
          return(f);
    }
    return(0);
}

int64_t IsGatewaysvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
    char destaddr[64];
    if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
    {
        if ( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
            return(tx.vout[v].nValue);
    }
    return(0);
}

bool GatewaysExactAmounts(struct CCcontract_info *cp,Eval* eval,const CTransaction &tx,int32_t minage,uint64_t txfee)
{
    static uint256 zerohash;
    CTransaction vinTx; uint256 hashBlock,activehash; int32_t i,numvins,numvouts; int64_t inputs=0,outputs=0,assetoshis;
    numvins = tx.vin.size();
    numvouts = tx.vout.size();
    for (i=0; i<numvins; i++)
    {
        //fprintf(stderr,"vini.%d\n",i);
        if ( (*cp->ismyvin)(tx.vin[i].scriptSig) != 0 )
        {
            //fprintf(stderr,"vini.%d check mempool\n",i);
            if ( eval->GetTxUnconfirmed(tx.vin[i].prevout.hash,vinTx,hashBlock) == 0 )
                return eval->Invalid("cant find vinTx");
            else
            {
                //fprintf(stderr,"vini.%d check hash and vout\n",i);
                if ( hashBlock == zerohash )
                    return eval->Invalid("cant Gateways from mempool");
                if ( (assetoshis= IsGatewaysvout(cp,vinTx,tx.vin[i].prevout.n)) != 0 )
                    inputs += assetoshis;
            }
        }
    }
    for (i=0; i<numvouts; i++)
    {
        //fprintf(stderr,"i.%d of numvouts.%d\n",i,numvouts);
        if ( (assetoshis= IsGatewaysvout(cp,tx,i)) != 0 )
            outputs += assetoshis;
    }
    if ( inputs != outputs+txfee )
    {
        fprintf(stderr,"inputs %llu vs outputs %llu\n",(long long)inputs,(long long)outputs);
        return eval->Invalid("mismatched inputs != outputs + txfee");
    }
    else return(true);
}

static int32_t myIs_coinaddr_inmempoolvout(char *coinaddr)
{
    int32_t i,n; char destaddr[64];
    BOOST_FOREACH(const CTxMemPoolEntry &e,mempool.mapTx)
    {
        const CTransaction &tx = e.GetTx();
        if ( (n= tx.vout.size()) > 0 )
        {
            const uint256 &txid = tx.GetHash();
            for (i=0; i<n; i++)
            {
                Getscriptaddress(destaddr,tx.vout[i].scriptPubKey);
                if ( strcmp(destaddr,coinaddr) == 0 )
                {
                    fprintf(stderr,"found (%s) vout in mempool\n",coinaddr);
                    return(1);
                }
            }
        }
    }
    return(0);
}

uint256 GatewaysReverseScan(uint256 &txid,int32_t height,uint256 reforacletxid,uint256 batontxid)
{
    CTransaction tx; uint256 hash,mhash,bhash,hashBlock,oracletxid; int32_t len,len2,numvouts; int64_t val,merkleht; CPubKey pk; std::vector<uint8_t>data;
    txid = zeroid;
    char str[65];
    //fprintf(stderr,"start reverse scan %s\n",uint256_str(str,batontxid));
    while ( myGetTransaction(batontxid,tx,hashBlock) != 0 && (numvouts= tx.vout.size()) > 0 )
    {
        //fprintf(stderr,"check %s\n",uint256_str(str,batontxid));
        if ( DecodeOraclesData(tx.vout[numvouts-1].scriptPubKey,oracletxid,bhash,pk,data) == 'D' && oracletxid == reforacletxid )
        {
            //fprintf(stderr,"decoded %s\n",uint256_str(str,batontxid));
            if ( oracle_format(&hash,&merkleht,0,'I',(uint8_t *)data.data(),0,(int32_t)data.size()) == sizeof(int32_t) && merkleht == height )
            {
                len = oracle_format(&hash,&val,0,'h',(uint8_t *)data.data(),sizeof(int32_t),(int32_t)data.size());
                len2 = oracle_format(&mhash,&val,0,'h',(uint8_t *)data.data(),(int32_t)(sizeof(int32_t)+sizeof(uint256)),(int32_t)data.size());
                char str2[65]; fprintf(stderr,"found merkleht.%d len.%d len2.%d %s %s\n",(int32_t)merkleht,len,len2,uint256_str(str,hash),uint256_str(str2,mhash));
                if ( len == sizeof(hash)+sizeof(int32_t) && len2 == 2*sizeof(mhash)+sizeof(int32_t) && mhash != zeroid )
                {
                    txid = batontxid;
                    //fprintf(stderr,"set txid\n");
                    return(mhash);
                }
                else
                {
                    //fprintf(stderr,"missing hash\n");
                    return(zeroid);
                }
            } //else fprintf(stderr,"height.%d vs search ht.%d\n",(int32_t)merkleht,(int32_t)height);
            batontxid = bhash;
            //fprintf(stderr,"new hash %s\n",uint256_str(str,batontxid));
        } else break;
    }
    fprintf(stderr,"end of loop\n");
    return(zeroid);
}

int32_t GatewaysCointxidExists(struct CCcontract_info *cp,uint256 cointxid)
{
    char txidaddr[64]; std::string coin; int32_t numvouts; uint256 hashBlock;
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    CCtxidaddr(txidaddr,cointxid);
    SetCCtxids(addressIndex,txidaddr);
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++)
    {
        return(-1);
    }
    return(myIs_coinaddr_inmempoolvout(txidaddr));
}

/* Get the block merkle root for a proof
 * IN: proofData
 * OUT: merkle root
 * OUT: transaction IDS
 */
uint256 BitcoinGetProofMerkleRoot(const std::vector<uint8_t> &proofData, std::vector<uint256> &txids)
{
    CMerkleBlock merkleBlock;
    if (!E_UNMARSHAL(proofData, ss >> merkleBlock))
        return uint256();
    return merkleBlock.txn.ExtractMatches(txids);
}

int64_t GatewaysVerify(char *refdepositaddr,uint256 oracletxid,int32_t claimvout,std::string refcoin,uint256 cointxid,const std::string deposithex,std::vector<uint8_t>proof,uint256 merkleroot,CPubKey destpub)
{
    std::vector<uint256> txids; uint256 proofroot,hashBlock,txid = zeroid; CTransaction tx; std::string name,description,format;
    char destaddr[64],destpubaddr[64],claimaddr[64],str[65],str2[65]; int32_t i,numvouts; int64_t nValue = 0;
    
    if ( myGetTransaction(oracletxid,tx,hashBlock) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {
        fprintf(stderr,"GatewaysVerify cant find oracletxid %s\n",uint256_str(str,oracletxid));
        return(0);
    }
    if ( DecodeOraclesCreateOpRet(tx.vout[numvouts-1].scriptPubKey,name,description,format) != 'C' || name != refcoin )
    {
        fprintf(stderr,"GatewaysVerify mismatched oracle name %s != %s\n",name.c_str(),refcoin.c_str());
        return(0);
    }
    proofroot = BitcoinGetProofMerkleRoot(proof,txids);
    if ( proofroot != merkleroot )
    {
        fprintf(stderr,"GatewaysVerify mismatched merkleroot %s != %s\n",uint256_str(str,proofroot),uint256_str(str2,merkleroot));
        return(0);
    }
    if ( DecodeHexTx(tx,deposithex) != 0 )
    {
        Getscriptaddress(claimaddr,tx.vout[claimvout].scriptPubKey);
        Getscriptaddress(destpubaddr,CScript() << ParseHex(HexStr(destpub)) << OP_CHECKSIG);
        if ( strcmp(claimaddr,destpubaddr) == 0 )
        {
            for (i=0; i<numvouts; i++)
            {
                Getscriptaddress(destaddr,tx.vout[i].scriptPubKey);
                if ( strcmp(refdepositaddr,destaddr) == 0 )
                {
                    txid = tx.GetHash();
                    nValue = tx.vout[i].nValue;
                    break;
                }
            }
        } else fprintf(stderr,"claimaddr.(%s) != destpubaddr.(%s)\n",claimaddr,destpubaddr);
    }
    if ( txid == cointxid )
    {
        fprintf(stderr,"verified proof for cointxid in merkleroot\n");
        return(nValue);
    } else fprintf(stderr,"(%s) != (%s) or txid %s mismatch.%d or script mismatch\n",refdepositaddr,destaddr,uint256_str(str,txid),txid != cointxid);
    return(0);
}

int64_t GatewaysDepositval(CTransaction tx,CPubKey mypk)
{
    int32_t numvouts,claimvout,height; int64_t amount; std::string coin,deposithex; std::vector<CPubKey> publishers; std::vector<uint256>txids; uint256 bindtxid,cointxid; std::vector<uint8_t> proof; CPubKey claimpubkey;
    if ( (numvouts= tx.vout.size()) > 0 )
    {
        if ( DecodeGatewaysDepositOpRet(tx.vout[numvouts-1].scriptPubKey,bindtxid,coin,publishers,txids,height,cointxid,claimvout,deposithex,proof,claimpubkey,amount) == 'D' && claimpubkey == mypk )
        {
            // coin, bindtxid, publishers
            fprintf(stderr,"need to validate deposittxid more\n");
            return(amount);
        }
    }
    return(0);
}

int32_t GatewaysBindExists(struct CCcontract_info *cp,CPubKey gatewayspk,uint256 reftokenid)
{
    char markeraddr[64],depositaddr[64]; std::string coin; int32_t numvouts; int64_t totalsupply; uint256 tokenid,oracletxid,hashBlock; 
    uint8_t M,N,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys; CTransaction tx;
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    _GetCCaddress(markeraddr,EVAL_GATEWAYS,gatewayspk);
    SetCCtxids(addressIndex,markeraddr);
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++)
    {
        if ( myGetTransaction(it->first.txhash,tx,hashBlock) != 0 && (numvouts= tx.vout.size()) > 0 && DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey)=='B' )
        {
            if ( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) == 'B' )
            {
                if ( tokenid == reftokenid )
                {
                    fprintf(stderr,"trying to bind an existing tokenid\n");
                    return(1);
                }
            }
        }
    }
    BOOST_FOREACH(const CTxMemPoolEntry &e, mempool.mapTx)
    {
        const CTransaction &txmempool = e.GetTx();
        const uint256 &hash = txmempool.GetHash();

        if ((numvouts=txmempool.vout.size()) > 0 && DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey)=='B')
            if (DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) == 'B' &&
            tokenid == reftokenid)
                return(1);
    }

    return(0);
}

bool GatewaysValidate(struct CCcontract_info *cp,Eval *eval,const CTransaction &tx, uint32_t nIn)
{
    int32_t numvins,numvouts,preventCCvins,preventCCvouts,i,numblocks,height,claimvout; bool retval; uint8_t funcid,hash[32],K,M,N,taddr,prefix,prefix2;
    char str[65],destaddr[64],depositaddr[65],validationError[512];
    std::vector<uint256> txids; std::vector<CPubKey> pubkeys,publishers,tmppublishers; std::vector<uint8_t> proof; int64_t fullsupply,totalsupply,amount,tmpamount;  
    uint256 hashblock,txid,bindtxid,deposittxid,withdrawtxid,completetxid,tokenid,tmptokenid,oracletxid,bindtokenid,cointxid,tmptxid,merkleroot,mhash; CTransaction bindtx,tmptx;
    std::string refcoin,tmprefcoin,hex,name,description,format; CPubKey pubkey,tmppubkey,gatewayspk;

    // fprintf(stderr,"return true without gateways validation\n");
    // return(true);
    numvins = tx.vin.size();
    numvouts = tx.vout.size();
    preventCCvins = preventCCvouts = -1;
    if ( numvouts < 1 )
        return eval->Invalid("no vouts");
    else
    {
        // for (i=0; i<numvins; i++)
        // {
        //     if ( IsCCInput(tx.vin[0].scriptSig) == 0 )
        //     {
        //         return eval->Invalid("illegal normal vini");
        //     }
        // }
        //fprintf(stderr,"check amounts\n");
        // if ( GatewaysExactAmounts(cp,eval,tx,1,10000) == false )
        // {
        //     fprintf(stderr,"Gatewaysget invalid amount\n");
        //     return false;
        // }
        // else
        // {        
            // txid = tx.GetHash();
            // memcpy(hash,&txid,sizeof(hash));
        gatewayspk = GetUnspendable(cp,0);
            
            if ( (funcid = DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey)) != 0)
            {
                switch ( funcid )
                {
                    case 'B':
                        //vin.0: normal input
                        //vin.1: CC input of tokens
                        //vout.0: CC vout of gateways tokens to gateways tokens CC address
                        //vout.1: CC vout txfee marker                        
                        //vout.n-1: opreturn - 'B' tokenid coin totalsupply oracletxid M N pubkeys taddr prefix prefix2
                        return eval->Invalid("unexpected GatewaysValidate for gatewaysbind!");
                        break;
                    case 'D':
                        //vin.0: normal input                        
                        //vout.0: CC vout txfee marker to destination pubkey                       
                        //vout.1: normal output txfee marker to txidaddr                        
                        //vout.n-1: opreturn - 'D' bindtxid coin publishers txids height cointxid claimvout deposithex proof destpub amount
                        return eval->Invalid("unexpected GatewaysValidate for gatewaysdeposit!");                        
                        break;
                    case 'C':
                        //vin.0: normal input
                        //vin.1: CC input of gateways tokens
                        //vin.2: CC input of marker from gatewaysdeposit tx
                        //vout.0: CC vout of tokens from deposit amount to destinatoin pubkey
                        //vout.1: CC vout change of gateways tokens to gateways tokens CC address (if any)                     
                        //vout.n-1: opreturn - 'C' tokenid bindtxid coin deposittxid destpub amount
                        if ((numvouts=tx.vout.size()) < 1 || DecodeGatewaysClaimOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,refcoin,deposittxid,pubkey,amount)!='C')
                            return eval->Invalid("invalid gatewaysClaim OP_RETURN data!"); 
                        else if ( IsCCInput(tx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewaysClaim!");
                        else if ( IsCCInput(tx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewaysClaim!");
                        else if ( IsCCInput(tx.vin[2].scriptSig) == 0 )
                            return eval->Invalid("vin.2 is CC for gatewaysClaim!");
                        else if ( tx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewaysClaim!");
                        else if (myGetTransaction(bindtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaysbind txid!");
                        else if ((numvouts=tmptx.vout.size()) < 1 || DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,tmprefcoin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B')
                            return eval->Invalid("invalid gatewaysbind OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");
                        else if (tmptokenid!=tokenid)
                            return eval->Invalid("tokenid does not match tokenid from gatewaysbind");
                        else if ( N == 0 || N > 15 || M > N )
                            return eval->Invalid("invalid MofN in gatewaysbind");
                        else if (pubkeys.size()!=N)
                        {
                            sprintf(validationError,"not enough pubkeys(%ld) for N.%d gatewaysbind ",pubkeys.size(),N);
                            return eval->Invalid(validationError);
                        }
                        else if ( (fullsupply=CCfullsupply(tokenid)) != totalsupply )
                        {
                            sprintf(validationError,"Gateway bind.%s (%s) globaladdr.%s totalsupply %.8f != fullsupply %.8f\n",refcoin.c_str(),uint256_str(str,tokenid),cp->unspendableCCaddr,(double)totalsupply/COIN,(double)fullsupply/COIN);
                            return eval->Invalid(validationError);
                        }
                        else if (myGetTransaction(oracletxid,tmptx,hashblock) == 0 || (numvouts=tmptx.vout.size()) <= 0 )
                        {
                            sprintf(validationError,"cant find oracletxid %s\n",uint256_str(str,oracletxid));
                            return eval->Invalid(validationError);
                        }
                        else if ( DecodeOraclesCreateOpRet(tmptx.vout[numvouts-1].scriptPubKey,name,description,format) != 'C' )
                        {
                            sprintf(validationError,"mismatched oracle name %s != %s\n",name.c_str(),refcoin.c_str());
                            return eval->Invalid(validationError);
                        }
                        else if (format.size()!=3 || strncmp(format.c_str(),"Ihh",3)!=0)
                        {
                            sprintf(validationError,"illegal format %s != Ihh\n",format.c_str());
                            return eval->Invalid(validationError);
                        }
                        else if (komodo_txnotarizedconfirmed(bindtxid) == false)
                            return eval->Invalid("gatewaysbind tx is not yet confirmed(notarised)!");
                        else if (myGetTransaction(deposittxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaysdeposittxid!");
                        else if ((numvouts=tmptx.vout.size()) < 1 || DecodeGatewaysDepositOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptxid,tmprefcoin,tmppublishers,txids,height,cointxid,claimvout,hex,proof,tmppubkey,tmpamount) != 'D')
                            return eval->Invalid("invalid gatewaysdeposit OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in deposit tx");
                        else if (bindtxid!=tmptxid)
                            return eval->Invalid("bindtxid does not match to bindtxid from gatewaysdeposit");                                   
                        else if (tmpamount>totalsupply)
                            return eval->Invalid("deposit amount greater then bind total supply");                        
                        else if (komodo_txnotarizedconfirmed(deposittxid) == false)
                            return eval->Invalid("gatewaysdeposit tx is not yet confirmed(notarised)!");
                        else if (amount!=tmpamount)
                            return eval->Invalid("claimed amount different then deposit amount");
                        else if (tx.vout[0].nValue!=amount)
                            return eval->Invalid("claim amount not matching amount in opret");       
                        else if (pubkey!=tmppubkey)
                            return eval->Invalid("claim destination pubkey different than in deposit tx");
                        else 
                        {
                            int32_t m;                            
                            merkleroot = zeroid;
                            for (i=m=0; i<N; i++)
                            {
                                fprintf(stderr,"pubkeys[%d] %s\n",i,pubkey33_str(str,(uint8_t *)&pubkeys[i]));
                                if ( (mhash= GatewaysReverseScan(txid,height,oracletxid,OraclesBatontxid(oracletxid,pubkeys[i]))) != zeroid )
                                {
                                    if ( merkleroot == zeroid )
                                        merkleroot = mhash, m = 1;
                                    else if ( mhash == merkleroot )
                                        m++;
                                    publishers.push_back(pubkeys[i]);
                                    txids.push_back(txid);
                                }
                            }                            
                            if ( merkleroot == zeroid || m < N/2 )
                            {
                                sprintf(validationError,"couldnt find merkleroot for ht.%d %s oracle.%s m.%d vs n.%d\n",height,tmprefcoin.c_str(),uint256_str(str,oracletxid),m,N);                            
                                return eval->Invalid(validationError);
                            }
                            else if (GatewaysVerify(depositaddr,oracletxid,claimvout,tmprefcoin,cointxid,hex,proof,merkleroot,pubkey)!=amount)
                                return eval->Invalid("external deposit not verified\n");
                        }
                        break;
                    case 'W':
                        //vin.0: normal input
                        //vin.1: CC input of tokens        
                        //vout.0: CC vout txfee marker to gateways CC address                
                        //vout.1: CC vout of gateways tokens back to gateways tokens CC address                  
                        //vout.2: CC vout change of tokens back to owners pubkey (if any)                                                
                        //vout.n-1: opreturn - 'W' tokenid bindtxid refcoin withdrawpub amount
                        return eval->Invalid("unexpected GatewaysValidate for gatewaysWithdraw!");                 
                        break;
                    case 'P':
                        //vin.0: normal input
                        //vin.1: CC input of marker from previous tx (withdraw or partialsing)
                        //vout.0: CC vout txfee marker to gateways CC address                        
                        //vout.n-1: opreturn - 'P' withdrawtxid refcoin number_of_signs mypk hex
                        if ((numvouts=tx.vout.size()) > 0 && DecodeGatewaysPartialOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,refcoin,K,pubkey,hex)!='P')
                            return eval->Invalid("invalid gatewaysPartialSign OP_RETURN data!"); 
                        else if ( IsCCInput(tx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewaysPartialSign!");
                        else if ( IsCCInput(tx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewaysPartialSign!");
                        else if ( tx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewaysPartialSign!");
                        else if (myGetTransaction(withdrawtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid withdraw txid!");
                        else if ((numvouts=tmptx.vout.size()) > 0 && DecodeGatewaysWithdrawOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,tmprefcoin,pubkey,amount)!='W')
                            return eval->Invalid("invalid gatewayswithdraw OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");                        
                        else if ( IsCCInput(tmptx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewaysWithdraw!");
                        else if ( IsCCInput(tmptx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[1].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.1 is CC for gatewaysWithdraw!");
                        else if (tmptx.vout[1].nValue!=amount)
                            return eval->Invalid("amount in opret not matching tx tokens amount!");
                        else if (komodo_txnotarizedconfirmed(withdrawtxid) == false)
                            return eval->Invalid("gatewayswithdraw tx is not yet confirmed(notarised)!");
                        else if (myGetTransaction(bindtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaysbind txid!");
                        else if ((numvouts=tmptx.vout.size()) < 1 || DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,tmprefcoin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B')
                            return eval->Invalid("invalid gatewaysbind OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");
                        else if (tmptokenid!=tokenid)
                            return eval->Invalid("tokenid does not match tokenid from gatewaysbind");
                        else if (komodo_txnotarizedconfirmed(bindtxid) == false)
                            return eval->Invalid("gatewaysbind tx is not yet confirmed(notarised)!");  
                        else if (K>M)
                            return eval->Invalid("invalid number of signs!"); 
                        break;
                    case 'S':          
                        //vin.0: normal input              
                        //vin.1: CC input of marker from previous tx (withdraw or partialsing)
                        //vout.0: CC vout txfee marker to gateways CC address                       
                        //vout.n-1: opreturn - 'S' withdrawtxid refcoin hex
                        if ((numvouts=tx.vout.size()) > 0 && DecodeGatewaysCompleteSigningOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,refcoin,K,hex)!='S')
                            return eval->Invalid("invalid gatewayscompletesigning OP_RETURN data!"); 
                        else if ( IsCCInput(tx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewayscompletesigning!");
                        else if ( IsCCInput(tx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewayscompletesigning!");
                        else if ( tx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewayscompletesigning!");
                        else if (myGetTransaction(withdrawtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid withdraw txid!");
                        else if ((numvouts=tmptx.vout.size()) > 0 && DecodeGatewaysWithdrawOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,tmprefcoin,pubkey,amount)!='W')
                            return eval->Invalid("invalid gatewayswithdraw OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");                        
                        else if ( IsCCInput(tmptx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewaysWithdraw!");
                        else if ( IsCCInput(tmptx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[1].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.1 is CC for gatewaysWithdraw!");
                        else if (tmptx.vout[1].nValue!=amount)
                            return eval->Invalid("amount in opret not matching tx tokens amount!");
                        else if (komodo_txnotarizedconfirmed(withdrawtxid) == false)
                            return eval->Invalid("gatewayswithdraw tx is not yet confirmed(notarised)!");
                        else if (myGetTransaction(bindtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaysbind txid!");
                        else if ((numvouts=tmptx.vout.size()) < 1 || DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,tmprefcoin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B')
                            return eval->Invalid("invalid gatewaysbind OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");
                        else if (tmptokenid!=tokenid)
                            return eval->Invalid("tokenid does not match tokenid from gatewaysbind");
                        else if (komodo_txnotarizedconfirmed(bindtxid) == false)
                            return eval->Invalid("gatewaysbind tx is not yet confirmed(notarised)!");  
                        else if (K!=M)
                            return eval->Invalid("invalid number of signs!");
                        break;                    
                    case 'M':                        
                        //vin.0: CC input of gatewayscompletesigning tx marker to gateways CC address                                               
                        //vout.0: opreturn - 'M' withdrawtxid refcoin completetxid   
                        if ((numvouts=tx.vout.size()) > 0 && DecodeGatewaysMarkDoneOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,refcoin,completetxid)!='M')
                            return eval->Invalid("invalid gatewaysmarkdone OP_RETURN data!"); 
                        else if ( IsCCInput(tx.vin[0].scriptSig) == 0 )
                            return eval->Invalid("vin.0 is CC for gatewaysmarkdone!");
                        else if (myGetTransaction(completetxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaygatewayscompletesigning txid!");
                        else if ((numvouts=tmptx.vout.size()) > 0 && DecodeGatewaysCompleteSigningOpRet(tmptx.vout[numvouts-1].scriptPubKey,withdrawtxid,tmprefcoin,K,hex)!='S')
                            return eval->Invalid("invalid gatewayscompletesigning OP_RETURN data!"); 
                        else if ( IsCCInput(tmptx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewayscompletesigning!");
                        else if ( IsCCInput(tmptx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewayscompletesigning!");
                        else if ( tmptx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewayscompletesigning!");
                        else if (myGetTransaction(withdrawtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid withdraw txid!");
                        else if ((numvouts=tmptx.vout.size()) > 0 && DecodeGatewaysWithdrawOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,tmprefcoin,pubkey,amount)!='W')
                            return eval->Invalid("invalid gatewayswithdraw OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");                        
                        else if ( IsCCInput(tmptx.vin[0].scriptSig) != 0 )
                            return eval->Invalid("vin.0 is normal for gatewaysWithdraw!");
                        else if ( IsCCInput(tmptx.vin[1].scriptSig) == 0 )
                            return eval->Invalid("vin.1 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[0].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.0 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[1].scriptPubKey.IsPayToCryptoCondition() == 0 )
                            return eval->Invalid("vout.1 is CC for gatewaysWithdraw!");
                        else if ( tmptx.vout[1].nValue!=amount)
                            return eval->Invalid("amount in opret not matching tx tokens amount!"); 
                        else if (komodo_txnotarizedconfirmed(withdrawtxid) == false)
                            return eval->Invalid("gatewayswithdraw tx is not yet confirmed(notarised)!");
                        else if (myGetTransaction(bindtxid,tmptx,hashblock) == 0)
                            return eval->Invalid("invalid gatewaysbind txid!");
                        else if ((numvouts=tmptx.vout.size()) < 1 || DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,tmprefcoin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B')
                            return eval->Invalid("invalid gatewaysbind OP_RETURN data!"); 
                        else if (tmprefcoin!=refcoin)
                            return eval->Invalid("refcoin different than in bind tx");
                        else if (tmptokenid!=tokenid)
                            return eval->Invalid("tokenid does not match tokenid from gatewaysbind");
                        else if (komodo_txnotarizedconfirmed(bindtxid) == false)
                            return eval->Invalid("gatewaysbind tx is not yet confirmed(notarised)!");  
                        else if (K!=M)
                            return eval->Invalid("invalid number of signs!");
                        break;                    
                }
            }
            retval = PreventCC(eval,tx,preventCCvins,numvins,preventCCvouts,numvouts);
            if ( retval != 0 )
                fprintf(stderr,"Gatewaysget validated\n");
            else fprintf(stderr,"Gatewaysget invalid\n");
            return(retval);
        // }
    }
}
// end of consensus code

// helper functions for rpc calls in rpcwallet.cpp

int64_t AddGatewaysInputs(struct CCcontract_info *cp,CMutableTransaction &mtx,CPubKey pk,uint256 bindtxid,int64_t total,int32_t maxinputs)
{
    char coinaddr[64],depositaddr[64]; int64_t threshold,nValue,price,totalinputs = 0,totalsupply,amount; 
    CTransaction vintx,bindtx; int32_t vout,numvouts,n = 0; uint8_t M,N,evalcode,funcid,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; std::string refcoin,tmprefcoin; CPubKey withdrawpub,destpub;
    uint256 tokenid,txid,oracletxid,tmpbindtxid,tmptokenid,deposittxid,hashBlock;

    if ( GetTransaction(bindtxid,bindtx,hashBlock,false) != 0 )
    {
        if ((numvouts=bindtx.vout.size())!=0 && DecodeGatewaysBindOpRet(depositaddr,bindtx.vout[numvouts-1].scriptPubKey,tokenid,refcoin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) == 'B')
        {
            GetTokensCCaddress(cp,coinaddr,pk);
            SetCCunspents(unspentOutputs,coinaddr);
            threshold = total/(maxinputs+1);
            fprintf(stderr,"check %s for gateway inputs\n",coinaddr);
            for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
            {
                txid = it->first.txhash;
                vout = (int32_t)it->first.index;
                if ( it->second.satoshis < threshold )
                    continue;
                // for (j=0; j<mtx.vin.size(); j++)
                //     if ( txid == mtx.vin[j].prevout.hash && vout == mtx.vin[j].prevout.n )
                //         break;
                // if ( j != mtx.vin.size() )
                //     continue;
                if ( GetTransaction(txid,vintx,hashBlock,false) != 0 )
                {
                    funcid=DecodeGatewaysOpRet(vintx.vout[vintx.vout.size()-1].scriptPubKey);
                    if (vout==0 && funcid=='B' && bindtxid==txid && total != 0 && maxinputs != 0)
                    {
                        mtx.vin.push_back(CTxIn(txid,vout,CScript()));
                        totalinputs += it->second.satoshis;
                        n++;
                        if ( (total > 0 && totalinputs >= total) || (maxinputs > 0 && n >= maxinputs)) break;
                    }
                    else if (vout==1 && funcid=='W' && DecodeGatewaysWithdrawOpRet(vintx.vout[vintx.vout.size()-1].scriptPubKey,tmptokenid,tmpbindtxid,tmprefcoin,withdrawpub,amount) == 'W' &&
                        tmpbindtxid==bindtxid && tmprefcoin==refcoin && tmptokenid==tokenid && total != 0 && maxinputs != 0)
                    {
                        mtx.vin.push_back(CTxIn(txid,vout,CScript()));
                        totalinputs += it->second.satoshis;
                        n++;
                        if ( (total > 0 && totalinputs >= total) || (maxinputs > 0 && n >= maxinputs)) break;                
                    }
                    else if (vout==1 && funcid=='C' && DecodeGatewaysClaimOpRet(vintx.vout[vintx.vout.size()-1].scriptPubKey,tmptokenid,tmpbindtxid,tmprefcoin,deposittxid,destpub,amount) == 'C' &&
                        tmpbindtxid==bindtxid && tmprefcoin==refcoin && tmptokenid==tokenid && total != 0 && maxinputs != 0)
                    {
                        mtx.vin.push_back(CTxIn(txid,vout,CScript()));
                        totalinputs += it->second.satoshis;
                        n++;
                        if ( (total > 0 && totalinputs >= total) || (maxinputs > 0 && n >= maxinputs)) break;                
                    }           
                }
            }
            return(totalinputs);
        }
        else fprintf(stderr,"invalid GatewaysBind\n");
    }
    else fprintf(stderr,"can't find GatewaysBind txid\n");
    return(0);
}

std::string GatewaysBind(uint64_t txfee,std::string coin,uint256 tokenid,int64_t totalsupply,uint256 oracletxid,uint8_t M,uint8_t N,std::vector<CPubKey> pubkeys)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CTransaction oracletx; uint8_t taddr,prefix,prefix2; CPubKey mypk,gatewayspk; CScript opret; uint256 hashBlock;
    struct CCcontract_info *cp,*cpTokens,C,CTokens; std::string name,description,format; int32_t i,numvouts; int64_t fullsupply;
    char destaddr[64],coinaddr[64],myTokenCCaddr[64],str[65],*fstr;

    cp = CCinit(&C,EVAL_GATEWAYS);
    cpTokens = CCinit(&CTokens,EVAL_TOKENS);
    if (coin=="KMD")
    {
        taddr = 0;
        prefix = 60;
        prefix2 = 85;
    }
    else
    {
        fprintf(stderr,"set taddr, prefix, prefix2 for %s\n",coin.c_str());
        taddr = 0;
        prefix = 60;
        prefix2 = 85;
    }
    if ( N == 0 || N > 15 || M > N )
    {
        fprintf(stderr,"illegal M.%d or N.%d\n",M,N);
        return("");
    }
    if ( pubkeys.size() != N )
    {
        fprintf(stderr,"M.%d N.%d but pubkeys[%d]\n",M,N,(int32_t)pubkeys.size());
        return("");
    }
    for (i=0; i<N; i++)
    {
        Getscriptaddress(coinaddr,CScript() << ParseHex(HexStr(pubkeys[i])) << OP_CHECKSIG);
        if ( CCaddress_balance(coinaddr) == 0 )
        {
            fprintf(stderr,"M.%d N.%d but pubkeys[%d] has no balance\n",M,N,i);
            return("");
        }
    }
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pubkey2pk(Mypubkey());
    _GetCCaddress(myTokenCCaddr,EVAL_TOKENS,mypk);
    gatewayspk = GetUnspendable(cp,0);
    if ( _GetCCaddress(destaddr,EVAL_GATEWAYS,gatewayspk) == 0 )
    {
        fprintf(stderr,"Gateway bind.%s (%s) cant create globaladdr\n",coin.c_str(),uint256_str(str,tokenid));
        return("");
    }
    if ( (fullsupply=CCfullsupply(tokenid)) != totalsupply )
    {
        fprintf(stderr,"Gateway bind.%s (%s) globaladdr.%s totalsupply %.8f != fullsupply %.8f\n",coin.c_str(),uint256_str(str,tokenid),cp->unspendableCCaddr,(double)totalsupply/COIN,(double)fullsupply/COIN);
        return("");
    }
    if ( CCtoken_balance(myTokenCCaddr,tokenid) != totalsupply )
    {
        fprintf(stderr,"token balance on %s %.8f != %.8f\n",myTokenCCaddr,(double)CCtoken_balance(myTokenCCaddr,tokenid)/COIN,(double)totalsupply/COIN);
        return("");
    }
    if ( GetTransaction(oracletxid,oracletx,hashBlock,false) == 0 || (numvouts= oracletx.vout.size()) <= 0 )
    {
        fprintf(stderr,"cant find oracletxid %s\n",uint256_str(str,oracletxid));
        return("");
    }
    if ( DecodeOraclesCreateOpRet(oracletx.vout[numvouts-1].scriptPubKey,name,description,format) != 'C' )
    {
        fprintf(stderr,"mismatched oracle name %s != %s\n",name.c_str(),coin.c_str());
        return("");
    }
    if ( (fstr=(char *)format.c_str()) == 0 || strncmp(fstr,"Ihh",3) != 0 )
    {
        fprintf(stderr,"illegal format (%s) != (%s)\n",fstr,(char *)"Ihh");
        return("");
    }
    if ( GatewaysBindExists(cp,gatewayspk,tokenid) != 0 )
    {
        fprintf(stderr,"Gateway bind.%s (%s) already exists\n",coin.c_str(),uint256_str(str,tokenid));
        return("");
    }
    if ( AddNormalinputs(mtx,mypk,2*txfee,3) > 0 )
    {
        if (AddTokenCCInputs(cpTokens, mtx, mypk, tokenid, totalsupply, 64)>0)
        {
            mtx.vout.push_back(MakeTokensCC1vout(cp->evalcode,totalsupply,gatewayspk));       
            mtx.vout.push_back(MakeCC1vout(cp->evalcode,txfee,gatewayspk));
            return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysBindOpRet('B',tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2)));
        }
    }
    CCerror = strprintf("cant find enough inputs");
    fprintf(stderr,"%s\n", CCerror.c_str() );
    return("");
}

std::string GatewaysDeposit(uint64_t txfee,uint256 bindtxid,int32_t height,std::string refcoin,uint256 cointxid,int32_t claimvout,std::string deposithex,std::vector<uint8_t>proof,CPubKey destpub,int64_t amount)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CTransaction bindtx; CPubKey mypk; uint256 oracletxid,merkleroot,mhash,hashBlock,tokenid,txid;
    int64_t totalsupply; int32_t i,m,n,numvouts; uint8_t M,N,taddr,prefix,prefix2; std::string coin; struct CCcontract_info *cp,C;
    std::vector<CPubKey> pubkeys,publishers; std::vector<uint256>txids; char str[67],depositaddr[64],txidaddr[64];

    cp = CCinit(&C,EVAL_GATEWAYS);
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pubkey2pk(Mypubkey());
    //fprintf(stderr,"GatewaysDeposit ht.%d %s %.8f numpks.%d\n",height,refcoin.c_str(),(double)amount/COIN,(int32_t)pubkeys.size());
    if ( GetTransaction(bindtxid,bindtx,hashBlock,false) == 0 || (numvouts= bindtx.vout.size()) <= 0 )
    {
        fprintf(stderr,"cant find bindtxid %s\n",uint256_str(str,bindtxid));
        return("");
    }
    if ( DecodeGatewaysBindOpRet(depositaddr,bindtx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B' || refcoin != coin )
    {
        fprintf(stderr,"invalid coin - bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        return("");
    }
    n = (int32_t)pubkeys.size();
    merkleroot = zeroid;
    for (i=m=0; i<n; i++)
    {
        fprintf(stderr,"pubkeys[%d] %s\n",i,pubkey33_str(str,(uint8_t *)&pubkeys[i]));
        if ( (mhash= GatewaysReverseScan(txid,height,oracletxid,OraclesBatontxid(oracletxid,pubkeys[i]))) != zeroid )
        {
            if ( merkleroot == zeroid )
                merkleroot = mhash, m = 1;
            else if ( mhash == merkleroot )
                m++;
            publishers.push_back(pubkeys[i]);
            txids.push_back(txid);
        }
    }
    fprintf(stderr,"cointxid.%s m.%d of n.%d\n",uint256_str(str,cointxid),m,n);
    if ( merkleroot == zeroid || m < n/2 )
    {
        //uint256 tmp;
        //decode_hex((uint8_t *)&tmp,32,(char *)"90aedc2f19200afc9aca2e351438d011ebae8264a58469bf225883045f61917f");
        //merkleroot = revuint256(tmp);
        fprintf(stderr,"couldnt find merkleroot for ht.%d %s oracle.%s m.%d vs n.%d\n",height,coin.c_str(),uint256_str(str,oracletxid),m,n);
        return("");
    }
    if ( GatewaysCointxidExists(cp,cointxid) != 0 )
    {
        fprintf(stderr,"cointxid.%s already exists\n",uint256_str(str,cointxid));
        return("");
    }
    if ( GatewaysVerify(depositaddr,oracletxid,claimvout,coin,cointxid,deposithex,proof,merkleroot,destpub) != amount )
    {
        fprintf(stderr,"deposittxid didnt validate\n");
        return("");
    }
    if ( AddNormalinputs(mtx,mypk,3*txfee,4) > 0 )
    {
        mtx.vout.push_back(MakeCC1vout(cp->evalcode,txfee,destpub));
        mtx.vout.push_back(CTxOut(txfee,CScript() << ParseHex(HexStr(CCtxidaddr(txidaddr,cointxid))) << OP_CHECKSIG));
        return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysDepositOpRet('D',bindtxid,coin,publishers,txids,height,cointxid,claimvout,deposithex,proof,destpub,amount)));
    }
    fprintf(stderr,"cant find enough inputs\n");
    return("");
}

std::string GatewaysClaim(uint64_t txfee,uint256 bindtxid,std::string refcoin,uint256 deposittxid,CPubKey destpub,int64_t amount)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CTransaction tx; CPubKey mypk,gatewayspk,tmpdestpub; struct CCcontract_info *cp,C; uint8_t M,N,taddr,prefix,prefix2;
    std::string coin, deposithex; std::vector<CPubKey> msigpubkeys,publishers; int64_t totalsupply,depositamount,tmpamount,inputs,CCchange=0;
    int32_t numvouts,claimvout,height; std::vector<uint8_t> proof;
    uint256 hashBlock,tokenid,oracletxid,tmptxid,cointxid; char str[65],depositaddr[64],coinaddr[64],destaddr[64]; std::vector<uint256> txids;

    cp = CCinit(&C,EVAL_GATEWAYS);
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    if ( GetTransaction(bindtxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {
        fprintf(stderr,"cant find bindtxid %s\n",uint256_str(str,bindtxid));
        return("");
    }
    if ( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,msigpubkeys,taddr,prefix,prefix2) != 'B' || coin != refcoin )
    {
        fprintf(stderr,"invalid coin - bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        return("");
    }
    if ( GetTransaction(deposittxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {
        fprintf(stderr,"cant find deposittxid %s\n",uint256_str(str,bindtxid));
        return("");
    }
    if (DecodeGatewaysDepositOpRet(tx.vout[numvouts-1].scriptPubKey,tmptxid,coin,publishers,txids,height,cointxid,claimvout,deposithex,proof,tmpdestpub,tmpamount) != 'D' || coin != refcoin)
    {
        fprintf(stderr,"invalid coin - deposittxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        return("");
    }
    if (tmpdestpub!=destpub)
    {
        fprintf(stderr,"different destination pubkey from desdeposittxid\n");
        return("");
    }                                               
    if ( (depositamount=GatewaysDepositval(tx,mypk)) != amount )
    {
        fprintf(stderr,"invalid Gateways deposittxid %s %.8f != %.8f\n",uint256_str(str,deposittxid),(double)depositamount/COIN,(double)amount/COIN);
        return("");
    }
    if ( AddNormalinputs(mtx,mypk,txfee,3) > 0 )
    {
		if ((inputs=AddGatewaysInputs(cp, mtx, gatewayspk, bindtxid, amount, 60)) > 0)
        {
            if ( inputs > amount ) CCchange = (inputs - amount);
            mtx.vin.push_back(CTxIn(deposittxid,0,CScript()));
            mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS,amount,destpub));
            if ( CCchange != 0 ) mtx.vout.push_back(MakeTokensCC1vout(EVAL_GATEWAYS,CCchange,gatewayspk));     
            return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysClaimOpRet('C',tokenid,bindtxid,refcoin,deposittxid,destpub,amount)));
        }
    }
    CCerror = strprintf("cant find enough inputs or mismatched total");
    fprintf(stderr,"%s\n", CCerror.c_str() );
    return("");
}

std::string GatewaysWithdraw(uint64_t txfee,uint256 bindtxid,std::string refcoin,CPubKey withdrawpub,int64_t amount)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CTransaction tx; CPubKey mypk,gatewayspk,signerpk; uint256 txid,tokenid,hashBlock,oracletxid,tmptokenid,tmpbindtxid,withdrawtxid; int32_t vout,numvouts;
    int64_t nValue,totalsupply,inputs,CCchange=0; uint8_t funcid,K,M,N,taddr,prefix,prefix2; std::string coin,hex;
    std::vector<CPubKey> msigpubkeys; char depositaddr[64],str[65],coinaddr[64]; struct CCcontract_info *cp,C,*cpTokens,CTokens;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    cp = CCinit(&C,EVAL_GATEWAYS);
    cpTokens = CCinit(&CTokens,EVAL_TOKENS);
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp, 0);

    if( GetTransaction(bindtxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {
        fprintf(stderr,"cant find bindtxid %s\n",uint256_str(str,bindtxid));
        return("");
    }
    if( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,msigpubkeys,taddr,prefix,prefix2) != 'B' || coin != refcoin )
    {
        fprintf(stderr,"invalid bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        return("");
    }
    _GetCCaddress(coinaddr,EVAL_GATEWAYS,gatewayspk);
    SetCCunspents(unspentOutputs,coinaddr);
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;
        vout = (int32_t)it->first.index;
        nValue = (int64_t)it->second.satoshis;
        K=0;
        if ( vout == 0 && nValue == 10000 && GetTransaction(txid,tx,hashBlock,false) != 0 && (numvouts= tx.vout.size())>0 && 
            (funcid=DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey))!=0 && (funcid=='W' || funcid=='P'))
        {
            if (funcid=='W' && DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,tmpbindtxid,coin,withdrawpub,amount)=='W'
                && refcoin==coin && tmptokenid==tokenid && tmpbindtxid==bindtxid)
            {
                CCerror = strprintf("unable to create withdraw, another withdraw pending\n");
                fprintf(stderr,"%s\n", CCerror.c_str() );
                return("");
            }
            
            else if (funcid=='P' && DecodeGatewaysPartialOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,signerpk,hex)=='P' &&
                GetTransaction(withdrawtxid,tx,hashBlock,false)!=0 && (numvouts=tx.vout.size())>0 && DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,tmpbindtxid,coin,withdrawpub,amount)=='W'
                && refcoin==coin && tmptokenid==tokenid && tmpbindtxid==bindtxid)                    
            {
                CCerror = strprintf("unable to create withdraw, another withdraw pending\n");
                fprintf(stderr,"%s\n", CCerror.c_str() );
                return("");
            }
        }
    }
    if( AddNormalinputs(mtx, mypk, 3*txfee, 4) > 0 )
    {
		if ((inputs = AddTokenCCInputs(cpTokens, mtx, mypk, tokenid, amount, 60)) > 0)
        {
            if ( inputs > amount ) CCchange = (inputs - amount);
            mtx.vout.push_back(MakeCC1vout(EVAL_GATEWAYS,txfee,gatewayspk));
            mtx.vout.push_back(MakeTokensCC1vout(EVAL_GATEWAYS,amount,gatewayspk));                   
            if ( CCchange != 0 ) mtx.vout.push_back(MakeCC1vout(EVAL_TOKENS, CCchange, mypk));            
            return(FinalizeCCTx(0, cpTokens, mtx, mypk, txfee,EncodeGatewaysWithdrawOpRet('W',tokenid,bindtxid,refcoin,withdrawpub,amount)));
        }
        else
        {
            CCerror = strprintf("not enough balance of tokens for withdraw");
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
    }
    CCerror = strprintf("cant find enough normal inputs");
    fprintf(stderr,"%s\n", CCerror.c_str() );
    return("");
}

std::string GatewaysPartialSign(uint64_t txfee,uint256 lasttxid,std::string refcoin, std::string hex)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CPubKey mypk,withdrawpub,signerpk,gatewayspk; struct CCcontract_info *cp,C; CTransaction tx,tmptx;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs; char funcid,str[65],depositaddr[64];
    int32_t numvouts; uint256 withdrawtxid,hashBlock,bindtxid,tokenid,oracletxid,tmptokenid; std::string coin,tmphex; int64_t amount,totalsupply;
    uint8_t K=0,M,N,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys;

    cp = CCinit(&C,EVAL_GATEWAYS);
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    if (GetTransaction(lasttxid,tx,hashBlock,false)==0 || (numvouts= tx.vout.size())<=0
        || (funcid=DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey))==0 || (funcid!='W' && funcid!='P'))
    {
        CCerror = strprintf("can't find last tx %s\n",uint256_str(str,lasttxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    if (funcid=='W')
    {
        withdrawtxid=lasttxid;
        if (DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount)!='W' || refcoin!=coin)
        {
            CCerror = strprintf("invalid withdraw tx %s\n",uint256_str(str,lasttxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(bindtxid,tmptx,hashBlock,false)==0 || (numvouts=tmptx.vout.size())<=0)
        {
            CCerror = strprintf("can't find bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B'
            || refcoin!=coin || tokenid!=tmptokenid)
        {
            CCerror = strprintf("invalid bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
    }
    else if (funcid=='P')
    {
        if (DecodeGatewaysPartialOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,signerpk,tmphex)!='P'  || refcoin!=coin)
        {
            CCerror = strprintf("cannot decode partialsign tx opret %s\n",uint256_str(str,lasttxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(withdrawtxid,tmptx,hashBlock,false)==0 || (numvouts= tmptx.vout.size())<=0)
        {
            CCerror = strprintf("can't find withdraw tx %s\n",uint256_str(str,withdrawtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysWithdrawOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount)!='W'
            || refcoin!=coin)
        {
            CCerror = strprintf("invalid withdraw tx %s\n",uint256_str(str,lasttxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(bindtxid,tmptx,hashBlock,false)==0 || (numvouts=tmptx.vout.size())<=0)
        {
            CCerror = strprintf("can't find bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B'
            || refcoin!=coin || tokenid!=tmptokenid)
        {
            CCerror = strprintf("invalid bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
    }
    if (AddNormalinputs(mtx,mypk,txfee,3)==0) fprintf(stderr,"error adding funds for partialsign\n");    
    mtx.vin.push_back(CTxIn(tx.GetHash(),0,CScript()));
    mtx.vout.push_back(MakeCC1vout(EVAL_GATEWAYS,txfee,gatewayspk));    
    return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysPartialOpRet('P',withdrawtxid,refcoin,K+1,mypk,hex)));
}

std::string GatewaysCompleteSigning(uint64_t txfee,uint256 lasttxid,std::string refcoin,std::string hex)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CPubKey mypk,gatewayspk,signerpk,withdrawpub; struct CCcontract_info *cp,C; char funcid,str[65],depositaddr[64]; int64_t amount,totalsupply;
    std::string coin,tmphex; CTransaction tx,tmptx; uint256 withdrawtxid,hashBlock,tokenid,tmptokenid,bindtxid,oracletxid; int32_t numvouts;
    uint8_t K=0,M,N,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys;

    cp = CCinit(&C,EVAL_GATEWAYS);
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    if ( txfee == 0 )
        txfee = 10000;
    if (GetTransaction(lasttxid,tx,hashBlock,false)==0 || (numvouts= tx.vout.size())<=0
        || (funcid=DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey))==0 || (funcid!='W' && funcid!='P'))
    {
        CCerror = strprintf("invalid last txid %s\n",uint256_str(str,lasttxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    if (funcid=='W')
    {
        withdrawtxid=lasttxid;
        if (DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount)!='W' || refcoin!=coin)
        {
            CCerror = strprintf("cannot decode withdraw tx opret %s\n",uint256_str(str,lasttxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(bindtxid,tmptx,hashBlock,false)==0 || (numvouts=tmptx.vout.size())<=0)
        {
            CCerror = strprintf("can't find bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B'
            || refcoin!=coin || tokenid!=tmptokenid)
        {
            CCerror = strprintf("invalid bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
    }
    else if (funcid=='P')
    {
        if (DecodeGatewaysPartialOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,signerpk,tmphex)!='P' || refcoin!=coin)
        {
            CCerror = strprintf("cannot decode partialsign tx opret %s\n",uint256_str(str,lasttxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(withdrawtxid,tmptx,hashBlock,false)==0 || (numvouts=tmptx.vout.size())==0)
        {
            CCerror = strprintf("invalid withdraw txid %s\n",uint256_str(str,withdrawtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysWithdrawOpRet(tmptx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount)!='W' || refcoin!=coin)
        {
            printf("aaaaaaaaaaa\n");
            CCerror = strprintf("cannot decode withdraw tx opret %s\n",uint256_str(str,withdrawtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (GetTransaction(bindtxid,tmptx,hashBlock,false)==0 || (numvouts=tmptx.vout.size())<=0)
        {
            CCerror = strprintf("can't find bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
        else if (DecodeGatewaysBindOpRet(depositaddr,tmptx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B'
            || refcoin!=coin || tokenid!=tmptokenid)
        {
            CCerror = strprintf("invalid bind tx %s\n",uint256_str(str,bindtxid));
            fprintf(stderr,"%s\n", CCerror.c_str() );
            return("");
        }
    }
    if (AddNormalinputs(mtx,mypk,txfee,3)==0) fprintf(stderr,"error adding funds for completesigning\n");
    mtx.vin.push_back(CTxIn(lasttxid,0,CScript()));
    mtx.vout.push_back(MakeCC1vout(EVAL_GATEWAYS,txfee,gatewayspk));    
    return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysCompleteSigningOpRet('S',withdrawtxid,refcoin,K+1,hex)));
}

std::string GatewaysMarkDone(uint64_t txfee,uint256 completetxid,std::string refcoin)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CPubKey mypk; struct CCcontract_info *cp,C; char str[65],depositaddr[64]; CTransaction tx; int32_t numvouts;
    uint256 withdrawtxid,bindtxid,oracletxid,tokenid,tmptokenid,hashBlock; std::string coin,hex;
    uint8_t K,M,N,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys; int64_t amount,totalsupply; CPubKey withdrawpub;

    cp = CCinit(&C,EVAL_GATEWAYS);
    mypk = pubkey2pk(Mypubkey());    
    if ( txfee == 0 )
        txfee = 10000;
    if (GetTransaction(completetxid,tx,hashBlock,false)==0 || (numvouts= tx.vout.size())<=0)
    {
        CCerror = strprintf("invalid completesigning txid %s\n",uint256_str(str,completetxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    else if (DecodeGatewaysCompleteSigningOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,hex)!='S' || refcoin!=coin)
    {
        CCerror = strprintf("cannot decode completesigning tx opret %s\n",uint256_str(str,completetxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    else if (GetTransaction(withdrawtxid,tx,hashBlock,false)==0 || (numvouts= tx.vout.size())==0)
    {
        CCerror = strprintf("invalid withdraw txid %s\n",uint256_str(str,withdrawtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    else if (DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount)!='W' || refcoin!=coin)
    {
        CCerror = strprintf("cannot decode withdraw tx opret %s\n",uint256_str(str,withdrawtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    else if (GetTransaction(bindtxid,tx,hashBlock,false)==0 || (numvouts=tx.vout.size())<=0)
    {
        CCerror = strprintf("can't find bind tx %s\n",uint256_str(str,bindtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    else if (DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B'
        || refcoin!=coin || tokenid!=tmptokenid)
    {
        CCerror = strprintf("invalid bind tx %s\n",uint256_str(str,bindtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    mtx.vin.push_back(CTxIn(completetxid,0,CScript()));        
    return(FinalizeCCTx(0,cp,mtx,mypk,txfee,EncodeGatewaysMarkDoneOpRet('M',withdrawtxid,refcoin,completetxid)));
}

UniValue GatewaysPendingDeposits(uint256 bindtxid,std::string refcoin)
{
    UniValue result(UniValue::VOBJ),pending(UniValue::VARR); CTransaction tx; std::string coin,hex,pub; 
    CPubKey mypk,gatewayspk,destpub; std::vector<CPubKey> pubkeys,publishers; std::vector<uint256> txids;
    uint256 tmpbindtxid,hashBlock,txid,tokenid,oracletxid,cointxid; uint8_t M,N,taddr,prefix,prefix2;
    char depositaddr[65],coinaddr[65],str[65],destaddr[65],txidaddr[65]; std::vector<uint8_t> proof;
    int32_t numvouts,vout,claimvout,height; int64_t totalsupply,nValue,amount; struct CCcontract_info *cp,C;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    cp = CCinit(&C,EVAL_GATEWAYS);
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    _GetCCaddress(coinaddr,EVAL_GATEWAYS,mypk);
    if ( GetTransaction(bindtxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {        
        CCerror = strprintf("cant find bindtxid %s\n",uint256_str(str,bindtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    if ( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 'B' || refcoin != coin)
    {
        CCerror = strprintf("invalid bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }  
    SetCCunspents(unspentOutputs,coinaddr);    
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;
        vout = (int32_t)it->first.index;
        nValue = (int64_t)it->second.satoshis;
        if ( vout == 0 && nValue == 10000 && GetTransaction(txid,tx,hashBlock,false) != 0 && (numvouts=tx.vout.size())>0 && 
            DecodeGatewaysDepositOpRet(tx.vout[numvouts-1].scriptPubKey,tmpbindtxid,coin,publishers,txids,height,cointxid,claimvout,hex,proof,destpub,amount) == 'D'
            && tmpbindtxid==bindtxid && refcoin == coin && myIsutxo_spentinmempool(txid,vout) == 0)
        {   
            UniValue obj(UniValue::VOBJ);
            obj.push_back(Pair("cointxid",uint256_str(str,cointxid)));
            obj.push_back(Pair("deposittxid",uint256_str(str,txid)));  
            CCtxidaddr(txidaddr,txid);
            obj.push_back(Pair("deposittxidaddr",txidaddr));              
            _GetCCaddress(destaddr,EVAL_TOKENS,destpub);
            obj.push_back(Pair("tokens_destination_address",destaddr));
            pub=HexStr(destpub);
            obj.push_back(Pair("claim_pubkey",pub));
            obj.push_back(Pair("amount",(double)amount/COIN));
            obj.push_back(Pair("confirmed_or_notarized",komodo_txnotarizedconfirmed(txid)));        
            pending.push_back(obj);
        }
    }
    result.push_back(Pair("coin",refcoin));
    result.push_back(Pair("pending",pending));
    return(result);
}

UniValue GatewaysPendingWithdraws(uint256 bindtxid,std::string refcoin)
{
    UniValue result(UniValue::VOBJ),pending(UniValue::VARR); CTransaction tx; std::string coin,hex; CPubKey mypk,gatewayspk,withdrawpub,signerpk;
    std::vector<CPubKey> msigpubkeys; uint256 hashBlock,tokenid,txid,tmpbindtxid,tmptokenid,oracletxid,withdrawtxid; uint8_t K,M,N,taddr,prefix,prefix2;
    char funcid,depositaddr[65],coinaddr[65],tokensaddr[65],destaddr[65],str[65],withaddr[65],numstr[32],signeraddr[65],txidaddr[65];
    int32_t i,n,numvouts,vout,queueflag; int64_t totalsupply,amount,nValue; struct CCcontract_info *cp,C;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    cp = CCinit(&C,EVAL_GATEWAYS);
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    _GetCCaddress(coinaddr,EVAL_GATEWAYS,gatewayspk);
    GetTokensCCaddress(cp,tokensaddr,gatewayspk);
    if ( GetTransaction(bindtxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {
        CCerror = strprintf("cant find bindtxid %s\n",uint256_str(str,bindtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    if ( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,msigpubkeys,taddr,prefix,prefix2) != 'B' || refcoin != coin )
    {
        CCerror = strprintf("invalid bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    n = msigpubkeys.size();
    queueflag = 0;
    for (i=0; i<n; i++)
        if ( msigpubkeys[i] == mypk )
        {
            queueflag = 1;
            break;
        }    
    SetCCunspents(unspentOutputs,coinaddr);    
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;
        vout = (int32_t)it->first.index;
        nValue = (int64_t)it->second.satoshis;
        K=0;
        if ( vout == 0 && nValue == 10000 && GetTransaction(txid,tx,hashBlock,false) != 0 && (numvouts= tx.vout.size())>0 && 
            (funcid=DecodeGatewaysOpRet(tx.vout[numvouts-1].scriptPubKey))!=0 && (funcid=='W' || funcid=='P') && myIsutxo_spentinmempool(txid,vout) == 0)
        {
            if (funcid=='W')
            {
                if (DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,tmpbindtxid,coin,withdrawpub,amount)==0 || refcoin!=coin || tmptokenid!=tokenid || tmpbindtxid!=bindtxid) continue;
            }
            else if (funcid=='P')
            {
                if (DecodeGatewaysPartialOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,signerpk,hex)!='P' || GetTransaction(withdrawtxid,tx,hashBlock,false)==0
                    || (numvouts=tx.vout.size())<=0 || DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,tmpbindtxid,coin,withdrawpub,amount)!='W'
                    || refcoin!=coin || tmptokenid!=tokenid || tmpbindtxid!=bindtxid)
                    continue;                    
            }      
            Getscriptaddress(destaddr,tx.vout[1].scriptPubKey);
            Getscriptaddress(withaddr,CScript() << ParseHex(HexStr(withdrawpub)) << OP_CHECKSIG);
            if ( strcmp(destaddr,tokensaddr) == 0 )
            {
                UniValue obj(UniValue::VOBJ);
                obj.push_back(Pair("withdrawtxid",uint256_str(str,tx.GetHash())));
                CCtxidaddr(txidaddr,tx.GetHash());
                obj.push_back(Pair("withdrawtxidaddr",txidaddr));
                obj.push_back(Pair("withdrawaddr",withaddr));
                sprintf(numstr,"%.8f",(double)tx.vout[1].nValue/COIN);
                obj.push_back(Pair("amount",numstr));                
                obj.push_back(Pair("confirmed_or_notarized",komodo_txnotarizedconfirmed(tx.GetHash())));
                if ( queueflag != 0 )
                {
                    obj.push_back(Pair("depositaddr",depositaddr));
                    Getscriptaddress(signeraddr,CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG);
                    obj.push_back(Pair("signeraddr",signeraddr));
                }
                if (N>1)
                {
                    obj.push_back(Pair("number_of_signs",K));
                    obj.push_back(Pair("last_txid",uint256_str(str,txid)));
                    if (K>0) obj.push_back(Pair("hex",hex));
                }
                pending.push_back(obj);
            }
        }
    }
    result.push_back(Pair("coin",refcoin));
    result.push_back(Pair("pending",pending));
    result.push_back(Pair("queueflag",queueflag));
    return(result);
}

UniValue GatewaysProcessedWithdraws(uint256 bindtxid,std::string refcoin)
{
    UniValue result(UniValue::VOBJ),processed(UniValue::VARR); CTransaction tx; std::string coin,hex; 
    CPubKey mypk,gatewayspk,withdrawpub; std::vector<CPubKey> msigpubkeys;
    uint256 withdrawtxid,hashBlock,txid,tokenid,tmptokenid,oracletxid; uint8_t K,M,N,taddr,prefix,prefix2;
    char depositaddr[65],coinaddr[65],str[65],numstr[32],withaddr[65],txidaddr[65];
    int32_t i,n,numvouts,vout,queueflag; int64_t totalsupply,nValue,amount; struct CCcontract_info *cp,C;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    cp = CCinit(&C,EVAL_GATEWAYS);
    mypk = pubkey2pk(Mypubkey());
    gatewayspk = GetUnspendable(cp,0);
    _GetCCaddress(coinaddr,EVAL_GATEWAYS,gatewayspk);
    if ( GetTransaction(bindtxid,tx,hashBlock,false) == 0 || (numvouts= tx.vout.size()) <= 0 )
    {        
        CCerror = strprintf("cant find bindtxid %s\n",uint256_str(str,bindtxid));
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    if ( DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,msigpubkeys,taddr,prefix,prefix2) != 'B' || refcoin != coin)
    {
        CCerror = strprintf("invalid bindtxid %s coin.%s\n",uint256_str(str,bindtxid),coin.c_str());
        fprintf(stderr,"%s\n", CCerror.c_str() );
        return("");
    }
    n = msigpubkeys.size();
    queueflag = 0;
    for (i=0; i<n; i++)
        if ( msigpubkeys[i] == mypk )
        {
            queueflag = 1;
            break;
        }    
    SetCCunspents(unspentOutputs,coinaddr);    
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++)
    {
        txid = it->first.txhash;
        vout = (int32_t)it->first.index;
        nValue = (int64_t)it->second.satoshis;
        if ( vout == 0 && nValue == 10000 && GetTransaction(txid,tx,hashBlock,false) != 0 && (numvouts= tx.vout.size())>0 && 
            DecodeGatewaysCompleteSigningOpRet(tx.vout[numvouts-1].scriptPubKey,withdrawtxid,coin,K,hex) == 'S' && refcoin == coin && myIsutxo_spentinmempool(txid,vout) == 0)
        {   
            if (GetTransaction(withdrawtxid,tx,hashBlock,false) != 0 && (numvouts= tx.vout.size())>0
                && DecodeGatewaysWithdrawOpRet(tx.vout[numvouts-1].scriptPubKey,tmptokenid,bindtxid,coin,withdrawpub,amount) == 'W' || refcoin!=coin || tmptokenid!=tokenid)          
            {
                UniValue obj(UniValue::VOBJ);
                obj.push_back(Pair("completesigningtxid",uint256_str(str,txid)));
                obj.push_back(Pair("withdrawtxid",uint256_str(str,withdrawtxid)));  
                CCtxidaddr(txidaddr,withdrawtxid);
                obj.push_back(Pair("withdrawtxidaddr",txidaddr));              
                Getscriptaddress(withaddr,CScript() << ParseHex(HexStr(withdrawpub)) << OP_CHECKSIG);
                obj.push_back(Pair("withdrawaddr",withaddr));
                sprintf(numstr,"%.8f",(double)tx.vout[1].nValue/COIN);
                obj.push_back(Pair("amount",numstr));
                obj.push_back(Pair("hex",hex));                
                processed.push_back(obj);            
            }
        }
    }
    result.push_back(Pair("coin",refcoin));
    result.push_back(Pair("processed",processed));
    result.push_back(Pair("queueflag",queueflag));
    return(result);
}

UniValue GatewaysList()
{
    UniValue result(UniValue::VARR); std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex; struct CCcontract_info *cp,C; uint256 txid,hashBlock,oracletxid,tokenid; CTransaction vintx; std::string coin; int64_t totalsupply; char str[65],depositaddr[64]; uint8_t M,N,taddr,prefix,prefix2; std::vector<CPubKey> pubkeys;
    cp = CCinit(&C,EVAL_GATEWAYS);
    SetCCtxids(addressIndex,cp->unspendableCCaddr);
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++)
    {
        txid = it->first.txhash;
        if ( GetTransaction(txid,vintx,hashBlock,false) != 0 )
        {
            if ( vintx.vout.size() > 0 && DecodeGatewaysBindOpRet(depositaddr,vintx.vout[vintx.vout.size()-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 0 )
            {
                result.push_back(uint256_str(str,txid));
            }
        }
    }
    return(result);
}

UniValue GatewaysInfo(uint256 bindtxid)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    UniValue result(UniValue::VOBJ),a(UniValue::VARR); std::string coin; char str[67],numstr[65],depositaddr[64],gatewaystokens[64];
    uint8_t M,N; std::vector<CPubKey> pubkeys; uint8_t taddr,prefix,prefix2; uint256 tokenid,oracletxid,hashBlock; CTransaction tx;
    CPubKey Gatewayspk; struct CCcontract_info *cp,C; int32_t i; int64_t totalsupply,remaining;
  
    result.push_back(Pair("result","success"));
    result.push_back(Pair("name","Gateways"));
    cp = CCinit(&C,EVAL_GATEWAYS);
    Gatewayspk = GetUnspendable(cp,0);
    GetTokensCCaddress(cp,gatewaystokens,Gatewayspk);
    if ( GetTransaction(bindtxid,tx,hashBlock,false) != 0 )
    {
        depositaddr[0] = 0;
        if ( tx.vout.size() > 0 && DecodeGatewaysBindOpRet(depositaddr,tx.vout[tx.vout.size()-1].scriptPubKey,tokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2) != 0 && M <= N && N > 0 )
        {
            if ( N > 1 )
            {
                result.push_back(Pair("M",M));
                result.push_back(Pair("N",N));
                for (i=0; i<N; i++)
                    a.push_back(pubkey33_str(str,(uint8_t *)&pubkeys[i]));
                result.push_back(Pair("pubkeys",a));
            } else result.push_back(Pair("pubkey",pubkey33_str(str,(uint8_t *)&pubkeys[0])));
            result.push_back(Pair("coin",coin));
            result.push_back(Pair("oracletxid",uint256_str(str,oracletxid)));
            result.push_back(Pair("taddr",taddr));
            result.push_back(Pair("prefix",prefix));
            result.push_back(Pair("prefix2",prefix2));
            result.push_back(Pair("deposit",depositaddr));
            result.push_back(Pair("tokenid",uint256_str(str,tokenid)));
            sprintf(numstr,"%.8f",(double)totalsupply/COIN);
            result.push_back(Pair("totalsupply",numstr));
            remaining = CCtoken_balance(gatewaystokens,tokenid);
            sprintf(numstr,"%.8f",(double)remaining/COIN);
            result.push_back(Pair("remaining",numstr));
            sprintf(numstr,"%.8f",(double)(totalsupply - remaining)/COIN);
            result.push_back(Pair("issued",numstr));
        }
    }
    return(result);
}
