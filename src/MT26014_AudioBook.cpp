// MT26014-Akanksha Mishra
#include "bookmgmt/MT26014_AudioBook.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    AudioBook::AudioBook(std::string id, std::string title,
                         std::string publisher, int year,
                         Money pricePerSeat, std::string accessUrl,
                         LicenseModel license, Money platformFee,
                         std::string narrator, int durationMinutes)
        : ElectronicResource(std::move(id), std::move(title), std::move(publisher), year,
                             pricePerSeat, std::move(accessUrl), license, platformFee),
          narrator_(std::move(narrator)),
          durationMinutes_(durationMinutes)
    {
        if (durationMinutes_ <= 0)
            throw std::invalid_argument("duration must be positive");
    }

    void AudioBook::printDetails(std::ostream &os) const
    {
        ElectronicResource::printDetails(os); // access url, license, platform fee
        os << "  narrator: " << narrator_ << "\n"
           << "  duration: " << durationMinutes_ << " min\n";
    }

} // namespace bookmgmt