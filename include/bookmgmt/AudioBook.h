#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt
{

    // Base class choice (Q3): AudioBook derives from ElectronicResource.
    // An audiobook is a digital item that the library licenses for a number of
    // concurrent listeners (seats), exactly like an e-book or an online database.
    // Deriving from ElectronicResource therefore gives it, with no extra code,
    // isDigital() == true and the seat-based price (platform fee + unit price x seats).
    // Deriving from Resource directly would mean rewriting that digital and seat
    // behaviour, and deriving from Book would be wrong because Book carries
    // print-only data (edition, binding) and per-copy pricing. Only the audiobook's
    // own data (narrator, duration) and its category are added here.
    class AudioBook : public ElectronicResource
    {
    public:
        AudioBook(std::string id, std::string title,
                  std::string publisher, int year,
                  Money pricePerSeat, std::string accessUrl,
                  LicenseModel license, Money platformFee,
                  std::string narrator, int durationMinutes);

        const std::string &narrator() const { return narrator_; }
        int durationMinutes() const { return durationMinutes_; }

        ResourceCategory category() const override { return ResourceCategory::AudioBook; }

    protected:
        void printDetails(std::ostream &os) const override;

    private:
        std::string narrator_;
        int durationMinutes_;
    };

} // namespace bookmgmt