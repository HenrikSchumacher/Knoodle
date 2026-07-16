struct Subklutter
{
    LUT_T identified_lut;
    LUT_T unidentified_lut;
    AssociativeContainer<ID_T,KeySet_T> identified_buckets;
    std::vector<KeySet_T> unidentified_buckets;
    std::vector<Key_T> stack;
    
    Int crossing_count   = 0;
    ID_T unidentified_id = 0;
    
public:
    
    Subklutter() = delete;
    
    Subklutter( const Int crossing_count_ )
    :   crossing_count( crossing_count_ )
    {}
    
    ~Subklutter() = default;
    
    
    Size_T IdentifiedKeyCount() const
    {
        return identified_lut.size();
    }
    
    Size_T UnidentifiedKeyCount() const
    {
        return unidentified_lut.size();
    }
    
    
    bool InsertIdentifiedKey( cref<Key_T> key, ID_T id )
    {
        if constexpr ( debugQ )
        {
            logprint("InsertIdentifiedKey(" + std::string(ToString(key)) + "," + ToString(id) + ")");
        }

        if( CrossingCount(key) <= crossing_count )
        {
            identified_buckets[id].insert(key);
            identified_lut[key] = id;
            return false;
        }
        else
        {
            if constexpr ( debugQ )
            {
                logprint("CrossingCount(key) exceeds crossing_count. Key ignored.");
            }
            return true;
        }
    }
    
    bool InsertUnidentifiedKey( cref<Key_T> key )
    {
        [[maybe_unused]] auto tag = [](){ return MethodName("InsertUnidentifiedKey"); };
        
        if constexpr ( debugQ )
        {
            logprint("InsertUnidentifiedKey(" + std::string(ToString(key)) + ")");
        }

        if( CrossingCount(key) > crossing_count )
        {
            eprint(tag() + ": Diagram with too many crossings detected. Something must be wrong. Aborting.");
            TOOLS_DUMP(crossing_count);
            TOOLS_DUMP(CrossingCount(key));
            return true;
        }
        
        if( identified_lut.contains(key) )
        {
            // Nothing to be done here.
            return false;
        }
        
        if( !unidentified_lut.contains(key) )
        {
            unidentified_lut[key] = unidentified_buckets.size();
            unidentified_buckets.push_back(KeySet_T());
            unidentified_buckets.back().insert(key);
            return false;
        }
        
        // We have no problem with duplicate keys. MacLeod keys are made to represent many diagrams at once.
        return false;
    }
    
    
    template<bool cleanse_lutQ>
    void DeleteUnidentifiedBucket( Size_T idx )
    {
        if constexpr ( debugQ )
        {
            logprint(std::string("DeleteUnidentifiedBucket<")+ToString(cleanse_lutQ)+">(" + ToString(idx) + ")");
        }
        
        if constexpr ( cleanse_lutQ )
        {
            for( const Key_T & key : unidentified_buckets[idx] )
            {
                unidentified_lut.erase(key);
            }
        }
        
        const Size_T p = idx + ID_T(1);
        
        if( p == unidentified_buckets.size() )
        {
            unidentified_buckets.pop_back();
            while( !unidentified_buckets.empty() && unidentified_buckets.back().empty() )
            {
                unidentified_buckets.pop_back();
            }
        }
        else if( p < unidentified_buckets.size() )
        {
            unidentified_buckets[idx] = KeySet_T();
        }
    }
    
    void MergeIntoIdentifiedBucket( Size_T idx, ID_T id )
    {
        if constexpr ( debugQ )
        {
            logprint("MergeIntoIdentifiedBucket(" + ToString(idx) + "," + ToString(id) + ")");
        }

        if constexpr ( debugQ )
        {
            logvalprint("unidentified_buckets[idx].size()",unidentified_buckets[idx].size());
            logvalprint("identified_buckets[id].size()",identified_buckets[id].size());
        }
        
        mref<KeySet_T> bucket = identified_buckets[id];
        
        for( const Key_T & key : unidentified_buckets[idx] )
        {
            if constexpr ( debugQ )
            {
                logvalprint("key in unidentified_buckets[idx]",key);
                logvalprint("unidentified_lut[key]",unidentified_lut[key]);
            }
            bucket.insert(key);
            identified_lut[key] = id;
        }
        
        if constexpr ( debugQ )
        {
            logprint("DeleteBucket<true>(idx);");
        }
        
        DeleteUnidentifiedBucket<true>(idx);
    }
    
    Size_T MergeIntoUnidentifiedBucket( Size_T idx_0, Size_T idx_1 )
    {
        if constexpr ( debugQ )
        {
            logprint("MergeIntoUnidentifiedBucket(" + ToString(idx_0) + "," + ToString(idx_1) + ")");
        }
        
        // We always merge into the smaller id.
        // We return the id of the persisting bucket.
        
        if( idx_0 == idx_1 ) { return idx_0; }
        
        auto [i_0,i_1] = MinMax(idx_0,idx_1);
        
        if constexpr ( debugQ )
        {
            logvalprint("i_0",i_0);
            logvalprint("i_1",i_1);
            
            // TODO: We might want to swap the buckets so that buckets[i_0] is the smaller bucket.
            // TODO: Alas, we have to be careful: just swapping the buckets is not enough; we also have to edit the lut entries. Bad idea!
        }
        
        if constexpr ( debugQ )
        {
            logvalprint("unidentified_buckets[i_0].size()",unidentified_buckets[i_0].size());
            logvalprint("unidentified_buckets[i_1].size()",unidentified_buckets[i_1].size());
        }
        
        KeySet_T & bucket = unidentified_buckets[i_0];
        
        for( const Key_T & key : unidentified_buckets[i_1] )
        {
            if constexpr ( debugQ )
            {
                logvalprint("key in bucket[i_1]",key);
                logvalprint("unidentified_lut[key]",unidentified_lut[key]);
            }
            bucket.insert(key);
            unidentified_lut[key] = i_0;
        }
        
        if constexpr ( debugQ )
        {
            logvalprint("i_0",i_0);
            logvalprint("i_1",i_1);
            logprint("DeleteBucket<false>(i_1);");
        }
        
        // We do not want to erase the lut-entries that belong to this bucket because we have already set them to th correct values.
        DeleteUnidentifiedBucket<false>(i_1);
        
        return i_0;
    }
    
#include "Subklutter/GenerateIdentified.hpp"
#include "Subklutter/GenerateUnidentified.hpp"
    
    
    bool Compute(
        mref<Reapr_T> reapr,
        const Size_T max_iter,
        const Size_T embedding_trials,
        const Size_T rotation_trials
    )
    {
        Size_T iter = 0;
        
        while( (iter < max_iter) && (UnidentifiedKeyCount() > Size_T(0)) )
        {
            Flag_T flag = GenerateUnidentified(reapr, embedding_trials, rotation_trials);
            
            if( flag == Flag_T::Failure )
            {
                return false;
            }
            ++iter;
        }
        
        return (UnidentifiedKeyCount() == Size_T(0));
    }
    
public:
    
    static std::string MethodName( const std::string & tag )
    {
        return ClassName() + "::" + tag;
    }
    
    static std::string ClassName()
    {
        return std::string("Subklutter")
        + "<" + TypeName<Int>
        + ">";
    }
    
};
