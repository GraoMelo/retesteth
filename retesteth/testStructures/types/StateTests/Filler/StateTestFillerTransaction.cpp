#include "StateTestFillerTransaction.h"
#include <retesteth/EthChecks.h>
#include <retesteth/TestHelper.h>
#include <retesteth/testStructures/Common.h>

namespace test
{
namespace teststruct
{
StateTestFillerTransaction::StateTestFillerTransaction(DataObject const& _data)
{
    try
    {
        spDataObject tmpD(new DataObject());
        (*tmpD).copyFrom(_data);
        (*tmpD).removeKey("data");
        (*tmpD).performModifier(mod_valueToCompactEvenHexPrefixed);

        // Read data from JSON in test filler
        // The VALUE fields can be decimal -> convert it to hex
        // The data field can be LLL or other code -> compile it to BYTES
        if (_data.atKey("to").asString().empty())
            m_creation = true;
        else
        {
            m_creation = false;
            spDataObject dTo(new DataObject());
            (*dTo).copyFrom(_data.atKey("to"));
            string const& to = _data.atKey("to").asString();
            if (to.size() > 1 && to[1] != 'x')
                (*dTo) = "0x" + _data.atKey("to").asString();
            m_to = spFH20(new FH20(dTo));
        }
        m_secretKey = spFH32(new FH32(tmpD->atKey("secretKey")));
        m_nonce = spVALUE(new VALUE(tmpD->atKey("nonce")));

        for (auto const& el2 : _data.atKey("data").getSubObjects())
        {
            DataObject const& el = el2.getCContent();
            spDataObject dataInKey(new DataObject());
            spAccessList accessList;
            if (el.type() == DataType::Object)
            {
                (*dataInKey).copyFrom(el.atKey("data"));
                accessList = spAccessList(new AccessList(el.atKey("accessList")));
                requireJsonFields(el, "StateTestFillerTransaction::dataWithList " + _data.getKey(),
                    {{"data", {{DataType::String}, jsonField::Required}},
                     {"accessList", {{DataType::Array}, jsonField::Required}}});
            }
            else
                (*dataInKey).copyFrom(el);

            // -- Compile LLL in transaction data into byte code if not already
            (*dataInKey).setKey("`data` array element in General Transaction Section");  // Hint

            string label;
            string rawData = dataInKey->asString();
            std::string const c_labelPrefix = ":label";
            if (rawData.find(c_labelPrefix) != string::npos)
            {
                size_t const pos = rawData.find(c_labelPrefix);
                size_t const posEnd = rawData.find(' ', pos + c_labelPrefix.size() + 1);
                if (posEnd != string::npos)
                {
                    label = rawData.substr(pos, posEnd - pos);
                    rawData = rawData.substr(posEnd + 1);  // remove label before code parsing
                }
                else
                {
                    label = rawData.substr(pos);
                    rawData = "";
                }
            }
            (*dataInKey).setString(test::compiler::replaceCode(rawData));
            // ---
            m_databox.push_back(Databox(dataInKey.getContent(), label, rawData.substr(0, 20), accessList));
        }
        for (auto const& el : tmpD->atKey("gasLimit").getSubObjects())
            m_gasLimit.push_back(el.getCContent());
        for (auto const& el : tmpD->atKey("value").getSubObjects())
            m_value.push_back(el.getCContent());

        if (tmpD->count("maxFeePerGas") || tmpD->count("maxPriorityFeePerGas"))
        {
            // EIP 1559 TRANSACTION TEMPLATE (gtest FILLER)
            m_maxFeePerGas = spVALUE(new VALUE(tmpD->atKey("maxFeePerGas")));
            m_maxPriorityFeePerGas = spVALUE(new VALUE(tmpD->atKey("maxPriorityFeePerGas")));
            requireJsonFields(_data, "StateTestFillerTransaction " + _data.getKey(),
                {{"data", {{DataType::Array}, jsonField::Required}},
                 {"gasLimit", {{DataType::Array}, jsonField::Required}},
                 {"nonce", {{DataType::String}, jsonField::Required}},
                 {"value", {{DataType::Array}, jsonField::Required}},
                 {"to", {{DataType::String}, jsonField::Required}},
                 {"maxFeePerGas", {{DataType::String}, jsonField::Required}},
                 {"maxPriorityFeePerGas", {{DataType::String}, jsonField::Required}},
                 {"secretKey", {{DataType::String}, jsonField::Required}}});
        }
        else
        {
            // LEGACY TRANSACTION TEMPLATE (gtest FILLER)
            m_gasPrice = spVALUE(new VALUE(tmpD->atKey("gasPrice")));
            requireJsonFields(_data, "StateTestFillerTransaction " + _data.getKey(),
                {{"data", {{DataType::Array}, jsonField::Required}},
                 {"gasLimit", {{DataType::Array}, jsonField::Required}},
                 {"gasPrice", {{DataType::String}, jsonField::Required}},
                 {"nonce", {{DataType::String}, jsonField::Required}},
                 {"value", {{DataType::Array}, jsonField::Required}},
                 {"to", {{DataType::String}, jsonField::Required}},
                 {"secretKey", {{DataType::String}, jsonField::Required}}});
        }
    }
    catch (std::exception const& _ex)
    {
        throw UpwardsException(string("StateTestFillerTransaction parse error: ") + _ex.what() + "\n" + _data.asJson());
    }
}

}  // namespace teststruct
}  // namespace test
