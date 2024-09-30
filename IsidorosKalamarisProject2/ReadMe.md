Έχει υλοποιηθεί το 100% της εργασίας. Μεγάλο μέρος της λογικής έμεινε όμοια με τη πρώτη εργασία οπότε στο ReadMe
θα δοθεί έμφαση στην υλοποίηση του συγχρονισμού, πως/που/πότε χρησιμοποιώ conditional variables πότε κλειδώνω
/ξεκλειδώνω mutexes κλπ.

Σημείωση: Χρησιμοποιώ 2 mutexes και  4 conditional_variables, τα οποία εξασφαλίζουν την ορθή λειτουργία της exit του concurrency 
καθώς και των περιορισμών που έχουμε με το buffersize/threadpoolsize.


Ας ξεκινήσουμε. Το main thread περιλαμβάνει τη δημιουργία των worker_threadπου ισούται με το buffersize.
Έπειτα μπαίνει στη κεντρική while ώστε να δέχεται κάθε καινούριο connection πού κάνει ο commander και δημιουργεί τα 
controller threads παιρνώντας τους το struct queuejob που είναι η τριπλέτα της κάθε διεργασίας (πριν δημιουργηθεί το
 controller_thread έχουμε ήδη περάσει στη τριπλέτα το ανάλογοfile_descriptor για το socket).


Ας δούμε παραπάνω τα δύο thread_functions που υπάρχουν, worker_thread και controller. Τα workers ξεκινάνε πριν 
γίνει κάποιο connection με τον client και περιμένουν μέχρι να μπει έστω και μια δουλειά μέσα στον buffer(ή μέχρι να μειωθούν τα 
ruuning processes ώστε να διατηρηθεί το Concurrency που έχει γίνει set από κάποιον client). Όταν συμβεί ένα από τα 2 τα worker_threads
ξεκινάνε να ξυπνάνε ένα ένα. Όταν κάνουν dequeue μια εργασία τότε στέλνουν σήμα στο controller (σε περίπτωση που είχε γίνει
wait για να διατηρηθεί η ουρά bufferisze). Έπειτα δομούμε τον πίνακα με string char**jobexec και εκτελούμε fork() Μέσα στη
fork κάνουμε ανακατεύθυνση της εξόδου ώστε το αποτέλεσμα της εργασίας που δώσαμε να φανεί σε κάποιο αρχείο pid.output. Έπειτα
διαβάζουμε αυτό το αρχείο και στέλνουμε το περιεχόμενο (δηλαδή το αποτέλεσμα του command που εκτελέστηκε) πίσω στον client.


Όσο αφορά το controller_thread αν το μέγεθος της ουράς (buffer) έχει φτάσει το μέγεθος του buffersize τότε γίνεται wait,
(εξ ού και το signal μετά από dequeue εργασίας στο worker_thread). Όποτε γίνεται κάποιο enqueue στον buffer τότε στέλνεται 
σήμα να ξυπνήσει κάποιο worker_thread. Επίσης το controller_thread kάνει implement το setConcurrency κάνωντας brodcast όλα τα 
worker_threads ώστε να τρέξουν όσα παραπάνω γίνονται χωρίς όμως να ξεπεράσουν το concurrency. Τέλος το exit χρησιμοποιεί 2 waits, το πρώτο γίνεται signal οταν πλέον η ουρά έχει μείνει κενή και το flag_variable=0 και το 2ο signal συμβαίνει μετά το write του αποτελέσματος της εργασίας στο socket.

Υπάρχουν πολύ αναλυτικά σχόλια για τα conditional_variables και στον ίδιο των κώδικα.

Επίσης, όσο αφορά τη διοργάνωση των φακέλων έχω αναφέρει μόνο το bin και το src file στο makefile καθώς δεν μου δημιουργόντουσαν .o αρχεία για το build, δεν χρησιμοποίησα third party libraries και δεν πρόσθεσα δικά μου τεστς. 

Υποσημείωση: Αν δοθεί δουλειά για εκτέλεση η οποία δεν επιστρέφει κάποιο αποτέλεσμα τότε στο commander θα φανούν gibberish χαρακτήρες λόγο του οτι δεν θα έχει τι να διαβάσει και στέλνει απλά τα bytes του μηνύματος που έκανε malloc χωρίς να έχει γράψει κάτι πάνω τους..



Τέλος, μια ενδεικτική εκτέλεση:

Ενεργοποίηση του σέρβερ: "./bin/jobExecutorServer 9001 2 4"
Ενεργοποίηση του client για,

issueJob: "./bin/jobCommander linux07.di.uoa.gr 9001 issueJob ./bin/sleepingtime &" (το θέλουμε σε background mode λόγο του πως λειτουργεί το dup2)

stop:  "./bin/jobCommander linux07.di.uoa.gr 9001 stop job_1"

poll: "./bin/jobCommander linux07.di.uoa.gr 9001 poll"

setConcurrency:" ./bin/jobCommander linux07.di.uoa.gr 9001 setConcurrency 4"

exit: ./bin/jobCommander linux07.di.uoa.gr 9001 exit


